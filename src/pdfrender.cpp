#include <algorithm>
#include <memory>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-page.h>
#include <pybind11/pybind11.h>
using namespace std;
namespace py = pybind11;

class PDFDocument
{
public:
    static PDFDocument frombytes(py::buffer bytes)
    {
        py::buffer_info info = bytes.request();
        char* const file_bytes = new char[info.size];
        std::copy_n(reinterpret_cast<char*>(info.ptr), info.size, file_bytes);
        auto document = poppler::document::load_from_raw_data(file_bytes, info.size);
        if (!document || document->is_locked())
            throw std::invalid_argument("invalid pdf file");

        return PDFDocument(document, file_bytes);
    }

    static PDFDocument open(const string& filename)
    {
        auto document = poppler::document::load_from_file(filename);
        if (!document || document->is_locked())
            throw std::invalid_argument("invalid pdf file");

        return PDFDocument(document, nullptr);
    }

    PDFDocument(const PDFDocument& other) = delete;

    PDFDocument(PDFDocument&& other)
        : doc(move(other.doc))
        , num_pages(other.num_pages)
        , file_bytes(move(other.file_bytes))
    {
        other.num_pages = 0;
    }

    void close()
    {
        this->file_bytes.reset();
        this->doc.reset();
        this->num_pages = 0;
    }

    bool closed() const
    {
        return !this->doc;
    }

    PDFDocument& __enter__()
    {
        return *this;
    }

    void __exit__(py::object, py::object, py::object)
    {
        this->close();
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
            mode = "1";
            buffer_size = ((image.width() + 7) / 8) * image.height();
            break;
        case poppler::image::format_gray8:
            mode = "L";
            buffer_size = image.width() * image.height();
            break;
        case poppler::image::format_rgb24:
            buffer_size = image.width() * image.height() * 3;
            mode = "RGB";
            break;
        case poppler::image::format_bgr24:
            buffer_size = image.width() * image.height() * 3;
            mode = "BGR";
            break;
        case poppler::image::format_argb32:
            buffer_size = image.width() * image.height() * 4;
            mode = "RGBA";
        }

        auto size = make_pair(image.width(), image.height());
        py::bytes buffer(image.data(), buffer_size);
        py::module Image = py::module::import("PIL.Image");

        auto pil_image = Image.attr("frombytes")(mode, size, buffer, "raw", "BGRA");
        auto info = pil_image.attr("info");
        info["dpi"] = make_pair(dpi, dpi);
        return pil_image;
    }

    size_t size() const
    {
        return this->num_pages;
    }

private:
    PDFDocument(poppler::document* document, const char* file_bytes)
        : doc(document)
        , num_pages(document->pages())
        , file_bytes(file_bytes)
    {}

    unique_ptr<poppler::document> doc;
    int num_pages = 0;
    unique_ptr<const char[]> file_bytes;
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

    const auto open_docstring =
        R"(Opens a PDF document given the file name.

    Args:
        filename: the filename to open.

    Returns:
        A PDFDocument object

    Raises:
        InvalidArgument exception if the file is an invalid pdf file, or if the file is password-locked.
)";

    const auto render_page_docstring =
        R"(Renders a page of the PDF document as a PIL image.

    Args:
        page_index: 0-based index of the page to render
        dpi (optional): the dpi to render the image at, defaults to 72

    Returns:
        The PIL image, or None if unsuccessful.
        The image can have mode '1', 'L', 'RGB', or 'RGBA'
)";

    py::class_<PDFDocument>(m, "PDFDocument")
        .def_static("frombytes", &PDFDocument::frombytes, frombytes_docstring, "bytes"_a)
        .def_static("open", &PDFDocument::open, open_docstring, "filename"_a)
        .def("close", &PDFDocument::close)
        .def_property_readonly("closed", &PDFDocument::closed)
        .def("__len__", &PDFDocument::size)
        .def("__enter__", &PDFDocument::__enter__)
        .def("__exit__", &PDFDocument::__exit__)
        .def("render_page", &PDFDocument::render_page, render_page_docstring, "page_index"_a, "dpi"_a = 72);

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#endif
}
