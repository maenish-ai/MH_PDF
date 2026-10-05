from pathlib import Path
import sys


def build_pdf() -> bytes:
    stream = b"BT /F1 18 Tf 72 770 Td (MaenPDF CI smoke test) Tj ET\n"
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>",
        b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
        b"<< /Length %d >>\nstream\n" % len(stream) + stream + b"endstream",
    ]
    out = bytearray(b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n")
    offsets = [0]
    for index, body in enumerate(objects, 1):
        offsets.append(len(out))
        out.extend(f"{index} 0 obj\n".encode("ascii"))
        out.extend(body)
        out.extend(b"\nendobj\n")
    xref = len(out)
    out.extend(f"xref\n0 {len(objects)+1}\n".encode("ascii"))
    out.extend(b"0000000000 65535 f \n")
    for offset in offsets[1:]:
        out.extend(f"{offset:010d} 00000 n \n".encode("ascii"))
    out.extend(
        (f"trailer\n<< /Size {len(objects)+1} /Root 1 0 R >>\n"
         f"startxref\n{xref}\n%%EOF\n").encode("ascii")
    )
    return bytes(out)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: create_smoke_pdf.py OUTPUT.pdf", file=sys.stderr)
        return 2
    path = Path(sys.argv[1])
    path.parent.mkdir(parents=True, exist_ok=True)
    data = build_pdf()
    path.write_bytes(data)
    if not data.startswith(b"%PDF-") or b"startxref" not in data:
        raise RuntimeError("generated smoke PDF is invalid")
    print(f"Wrote {path} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
