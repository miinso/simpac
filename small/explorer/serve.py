import http.server
import mimetypes
import os
import socketserver
import sys
from pathlib import Path


class CorsHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self) -> None:
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, HEAD, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "*")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Resource-Policy", "cross-origin")
        super().end_headers()

    def do_OPTIONS(self) -> None:
        self.send_response(204)
        self.end_headers()


def main() -> None:
    mimetypes.add_type("application/wasm", ".wasm")
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    root = Path(__file__).resolve().parent

    os.chdir(root)
    handler = CorsHandler
    with socketserver.TCPServer(("", port), handler) as httpd:
        print(f"Serving {root} on http://localhost:{port}/")
        print(
            "Explorer: https://www.flecs.dev/explorer/?host="
            f"http://localhost:{port}/explorer_demo.wasm"
        )
        httpd.serve_forever()


if __name__ == "__main__":
    main()
