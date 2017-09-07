#include <memory>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>

extern "C"
{

void* load_document_from_raw_data(const char* file_data, int file_data_length)
{
    auto doc = poppler::document::load_from_raw_data(file_data, file_data_length);
    if (doc && doc->is_locked())
    {
        delete doc;
        doc = nullptr;
    }
    return reinterpret_cast<void*>(doc);
}

void* load_document_from_file(const char* filename)
{
    auto doc = poppler::document::load_from_file(filename);
    if (doc && doc->is_locked())
    {
        delete doc;
        doc = nullptr;
    }
    return reinterpret_cast<void*>(doc);
}

void delete_document(void* doc)
{
    delete reinterpret_cast<poppler::document*>(doc);
}

int num_pages(void* doc)
{
    return reinterpret_cast<poppler::document*>(doc)->pages();
}

const char* render_page(void* doc, int index, int dpi, int* height, int* width, int* format)
{
    auto page = std::unique_ptr<poppler::page>(reinterpret_cast<poppler::document*>(doc)->create_page(index));
    if (!page)
        return nullptr;

    poppler::page_renderer renderer;
    renderer.set_render_hints(0x7);
    auto image = renderer.render_page(page.get(), dpi, dpi);
    if (!image.is_valid())
        return nullptr;

    *height = image.height();
    *width = image.width();
    *format = image.format();

    auto data_size = image.bytes_per_row() * image.height();
    auto image_data_copy = new char[data_size];
    std::copy(image.const_data(), image.const_data() + data_size, image_data_copy);

    return image_data_copy;
}

void delete_image_data(const char* data)
{
    delete data;
}

}
