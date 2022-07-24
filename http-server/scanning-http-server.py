from datetime import datetime
import os
import pathlib
import subprocess
import sys
import tempfile
from http.server import BaseHTTPRequestHandler, HTTPServer


class ServerHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(bytes("http server is ok", "utf-8"))
        elif self.path == "/color":
            today = datetime.now()

            print("now is {}".format(today))

            try:
                with tempfile.TemporaryDirectory(prefix="osbro") as tmp_dir:
                    print("Created temporary directory", tmp_dir)

                    raw_file = pathlib.Path(tmp_dir, "scanner.raw")

                    print("Created raw file", raw_file)

                    process_exit = subprocess.run(["../cmake-build-debug/osbro", "-f", raw_file, "-m", "c", "-r", "300"])
                    print("Return code of process exit is", process_exit.returncode)

                    if process_exit.returncode == 0:
                        subprocess.run(["../cmake-build-debug/raw2pnm", raw_file])

                        png_path = pathlib.Path(tmp_dir, "scanner.png")
                        with open(png_path, "wb") as png_file:
                            raw_file_name = str(raw_file)
                            ppm_file_name = raw_file_name[0:len(raw_file_name)-4] + ".ppm"

                            print("PPM file name is", ppm_file_name)

                            process_exit = subprocess.run(["pnmtopng", ppm_file_name], stdout=png_file)

                            print("Return code of `pnmtopng` process is", process_exit.returncode)

                            print("PNG file is", os.stat(png_file.name))

                        self.send_response(200)
                        self.send_header("Content-Type", "image/png")
                        self.send_header("Content-Length", str(os.stat(png_file.name).st_size))
                        self.send_header("Cache-Control", "no-store")
                        self.end_headers()

                        with open(png_path, "rb") as png_file:
                            while True:
                                buf = png_file.read(4096)
                                if buf:
                                    self.wfile.write(buf)
                                else:
                                    break
                    else:
                        self.send_response(503)
                        self.send_header("Content-Type", "text/plain")
                        self.end_headers()
                        self.wfile.write(
                            bytes("osbro returned error code {0}".format(process_exit.returncode), "utf-8"))
            except:
                print("handler exception", sys.exc_info()[0])

                self.send_response(503)
                self.send_header("Content-Type", "text/plain")
                self.end_headers()
                self.wfile.write(bytes(str(sys.exc_info()[0]), "utf-8"))


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("No port given", file=sys.stderr)
        exit(1)

    port = int(sys.argv[1])

    print("Starting http server at port {0}".format(port))

    server_address = ('', port)
    http_server = HTTPServer(server_address, ServerHandler)

    try:
        http_server.serve_forever()
    except KeyboardInterrupt:
        pass

    http_server.server_close()
    print("Server stopped")

    exit(0)
