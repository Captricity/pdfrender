from PIL import Image
from _libpdfrender import ffi, lib


class PDFDocument(object):
    def __init__(self):
        self.height_buffer = ffi.new('int*')
        self.width_buffer = ffi.new('int*')
        self.format_buffer = ffi.new('int*')

    @staticmethod
    def from_bytes(data):
        data = bytearray(data)
        document_object = lib.load_document_from_raw_data(
            ffi.from_buffer(data),
            ffi.cast('int', len(data))
        )
        if document_object == ffi.NULL:
            raise ValueError('invalid pdf data')

        document_object = ffi.gc(document_object, lib.delete_document)

        pdf_document = PDFDocument()
        pdf_document.document_object = document_object
        pdf_document._bytes_data = data
        pdf_document.num_pages = lib.num_pages(document_object)
        return pdf_document

    @staticmethod
    def from_file(filename):
        c_filename = ffi.new('char[]', filename)
        document_object = lib.load_document_from_file(c_filename)
        if document_object == ffi.NULL:
            raise ValueError('invalid pdf data')

        pdf_document = PDFDocument()
        pdf_document.document_object = document_object
        pdf_document.num_pages = lib.num_pages(document_object)
        return pdf_document

    def __len__(self):
        return self.num_pages

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        del self.document_object
        self._bytes_data = None

    def render_page(self, page_index, dpi=72):
        if page_index < 0 or page_index >= self.num_pages:
            raise IndexError('list index out of range')

        image_data = lib.render_page(
            self.document_object,
            ffi.cast('int', page_index),
            ffi.cast('int', dpi),
            self.height_buffer,
            self.width_buffer,
            self.format_buffer
        )

        if image_data == ffi.NULL:
            return None

        height = self.height_buffer[0]
        width = self.width_buffer[0]
        format = self.format_buffer[0]

        if format == 0:
            return None
        elif format == 1:
            mode = 'L'
            image_data_size = width * height
        elif format == 2:
            mode = 'RGB'
            image_data_size = width * height * 3
        elif format == 3:
            mode = 'RGBA'
            image_data_size = width * height * 4

        image_data_buffer = ffi.buffer(image_data, image_data_size)
        image = Image.frombytes(mode, (width, height), image_data_buffer, 'raw', 'BGRA')
        image.info['dpi'] = dpi

        lib.delete_image_data(image_data)
        return image
