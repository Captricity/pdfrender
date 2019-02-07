# pdfrender

Renders a PDF document as images directly using [poppler](https://poppler.freedesktop.org/).

## Example
```python
from pdfrender import PDFDocument

with PDFDocument.open('/path/to/myfile.pdf') as doc:
    for i in range(len(doc)):
        image = doc.render_page(i, dpi=200)  # image is a PIL image
        image.show()
```
