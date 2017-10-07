#include <memory>
#include <vector>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <pybind11/pybind11.h>
using namespace std;
namespace py = pybind11;


class PDFDocument
{
public:
    static PDFDocument frombytes(py::buffer bytes)
    {
        py::buffer_info info = bytes.request();
        vector<char> file_bytes(info.size);
        std::copy_n(reinterpret_cast<char*>(info.ptr), info.size, file_bytes.data());
        auto document = poppler::document::load_from_raw_data(file_bytes.data(), info.size);
        if (!document || document->is_locked())
            throw std::invalid_argument("invalid pdf data");

        return PDFDocument(document, move(file_bytes));
    }

    static PDFDocument fromfile(const string& filename)
    {
        auto document = poppler::document::load_from_file(filename);
        if (!document || document->is_locked())
            throw std::invalid_argument("invalid pdf data");

        return PDFDocument(document, {});
    }

    PDFDocument(const PDFDocument& other) = delete;

    PDFDocument(PDFDocument&& other):
        doc(move(other.doc)),
        num_pages(other.num_pages),
        file_bytes(move(other.file_bytes))
    {
        other.num_pages = 0;
    }

    PDFDocument& __enter__()
    {
        return *this;
    }

    void __exit__(py::object, py::object, py::object)
    {
        this->file_bytes.clear();
        this->doc.reset();
        this->num_pages = 0;
    }

    py::object render_page(int page_index, int dpi)
    {
        if (!this->doc)
            throw std::runtime_error("Invalid pdf document.");

        if (page_index < 0 || page_index >= this->num_pages)
            throw std::invalid_argument("page index out of range");

        std::unique_ptr<poppler::page> page(this->doc->create_page(page_index));
        poppler::page_renderer renderer;
        renderer.set_render_hints(0x7);
        auto image = renderer.render_page(page.get(), dpi, dpi);
        if (!image.is_valid())
            return py::none();

        string mode;
        ssize_t buffer_size;

        switch (image.format())
        {
        case poppler::image::format_invalid:
            return py::none();
        case poppler::image::format_mono:
            mode = "L";
            buffer_size = image.width() * image.height();
            break;
        case poppler::image::format_rgb24:
            buffer_size = image.width() * image.height() * 3;
            mode = "RGB";
            break;
        case poppler::image::format_argb32:
            buffer_size = image.width() * image.height() * 4;
            mode = "RGBA";
        }

        auto size = make_pair(image.width(), image.height());
        py::bytes buffer(image.data(), buffer_size);
        py::module Image = py::module::import("PIL.Image");
        return Image.attr("frombytes")(mode, size, buffer, "raw", "BGRA");
    }

    size_t size() const
    {
        return this->num_pages;
    }

private:
    PDFDocument(poppler::document* document, vector<char>&& file_bytes):
        doc(document), num_pages(document->pages()), file_bytes(move(file_bytes))
    {}

    unique_ptr<poppler::document> doc;
    int num_pages = 0;
    vector<char> file_bytes;
};


PYBIND11_MODULE(pdfrender, m)
{
    using namespace pybind11::literals;

    m.doc() = "PDF rendering";

    const auto frombytes_docstring =
R"(Opens a PDF document using the raw bytes of the PDF file.

    Args:
        bytes: The raw bytes of the PDF file.

    Returns:
        A PDFDocument object

    Raises:
        InvalidArgument exception if the file is invalid, or if the file is password-locked.
)";

    const auto fromfile_docstring =
R"(Opens a PDF document given the file name.

    Args:
        filename: the filename to open.

    Returns:
        A PDFDocument object

    Raises:
        InvalidArgument exception if the file is invalid, or if the file is password-locked.
)";

    const auto render_page_docstring =
R"(Renders a page of the PDF document as a PIL image.

    Args:
        page_index: 0-based index of the page to render
        dpi (optional): the dpi to render the image at

    Returns:
        The PIL image, or None if unsuccessful.
        The image can have mode 'L', 'RGB', or 'RGBA'
)";

    py::class_<PDFDocument>(m, "PDFDocument")
        .def_static("frombytes", &PDFDocument::frombytes, frombytes_docstring, "bytes"_a)
        .def_static("fromfile", &PDFDocument::fromfile, fromfile_docstring, "filename"_a)
        .def("__len__", &PDFDocument::size)
        .def("__enter__", &PDFDocument::__enter__)
        .def("__exit__", &PDFDocument::__exit__)
        .def("render_page", &PDFDocument::render_page, render_page_docstring,
             "page_index"_a, "dpi"_a=72);
}
