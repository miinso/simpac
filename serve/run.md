# Launch the wasm demo with `serve`

1. Build (or run the asset smoke test if you want Bazel to validate the bundle automatically):

	```powershell
	bazel build //examples:webapp
	```

2. Start the HTTPS file server from the repo root (adjust the port if 3000 is in use):

    ```
    npx serve bazel-bin -l 3000 -C -c serve.json --ssl-cert serve/cert.pem --ssl-key serve/key.pem --no-clipboard
    ```

	* `serve/serve.json` adds COOP/COEP headers so browsers allow WebAssembly modules that opt into advanced features.
	* The bundled `serve/cert.pem` and `serve/key.pem` let you test under HTTPS, which some browsers require for SharedArrayBuffer and pointer-lock APIs.

3. Open <https://localhost:3000/app1.html> in your browser. Accept the self-signed certificate when prompted.

To stop the server, press `Ctrl+C` in the terminal.
