import unittest
from pdfrender import PDFDocument
from PIL import Image


class PDFDocumentTestCase(unittest.TestCase):
    def test_open_from_file(self):
        doc = PDFDocument.fromfile('tests/assets/1page.pdf')
        self.assertIsNotNone(doc)
        self.assertEqual(len(doc), 1)

    def test_open_nonexisting_file(self):
        with self.assertRaisesRegexp(ValueError, 'invalid pdf file'):
            doc = PDFDocument.fromfile('tests/assets/missing.pdf')

    def test_open_corrupted_file(self):
        with self.assertRaisesRegexp(ValueError, 'invalid pdf file'):
            doc = PDFDocument.fromfile('tests/assets/corrupted.pdf')

    def test_open_from_bytes(self):
        with open('tests/assets/1page.pdf', 'rb') as f:
            file_bytes = f.read()
        doc = PDFDocument.frombytes(file_bytes)
        self.assertIsNotNone(doc)
        self.assertEqual(len(doc), 1)

    def test_open_from_corrupted_bytes(self):
        with self.assertRaisesRegexp(ValueError, 'invalid pdf file'):
            PDFDocument.frombytes(b'foo')

    def test_open_password_protected_file_fails(self):
        with self.assertRaisesRegexp(ValueError, 'invalid pdf file'):
            PDFDocument.fromfile('tests/assets/password_locked.pdf')

    def test_render_page(self):
        doc = PDFDocument.fromfile('tests/assets/gre_research_validity_data.pdf')
        self.assertEqual(len(doc), 4)
        page = doc.render_page(0)
        self.assertIsInstance(page, Image.Image)
        self.assertEqual(page.size, (614, 793))
        self.assertEqual(page.mode, 'RGBA')
        self.assertEqual(page.load()[613, 792], (249, 178, 10, 255))
        self.assertEqual(page.info['dpi'], (72, 72))

    def test_render_page_higher_dpi(self):
        doc = PDFDocument.fromfile('tests/assets/gre_research_validity_data.pdf')
        page = doc.render_page(0, dpi=150)
        self.assertEqual(page.size, (1279, 1653))

    def test_render_page_invalid_index(self):
        doc = PDFDocument.fromfile('tests/assets/gre_research_validity_data.pdf')
        with self.assertRaisesRegexp(ValueError, 'page index out of range'):
            doc.render_page(-1)
        with self.assertRaisesRegexp(ValueError, 'page index out of range'):
            doc.render_page(5)
