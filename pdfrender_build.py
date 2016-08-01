from cffi import FFI
ffi = FFI()

with open('src/pdfrender.cpp') as f:
    extra_compile_args = ['-std=c++1y']
    extra_link_args = ['-lpoppler-cpp']

    ffi.set_source('pdfrender._libpdfrender', f.read(),
                   extra_compile_args=extra_compile_args,
                   extra_link_args=extra_link_args)

ffi.cdef('''
void* load_document_from_raw_data(const char* file_data, int file_data_length);
void* load_document_from_file(const char* filename);
void delete_document(void* doc);
int num_pages(void* doc);
const char* render_page(void* doc, int index, int dpi, int* height, int* width, int* format);
void delete_image_data(const char* data);
''')


if __name__ == '__main__':
    ffi.compile()
