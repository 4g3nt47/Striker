# Striker C2

**Striker** is a simple *command and control* (C2) program.

## Disclaimer

This project is under active development. Most of the features are experimental, with more to come. Expect breaking changes.

## Features

**A) Agents**
- Native agents for linux and windows hosts.
- Self-contained, minimal python agent should you ever need it.
- HTTP(s) channels.
- Aynchronous tasks execution.
- Support for multiple redirectors, and can fallback to others when active one goes down.

**B) Backend / Teamserver**
- Supports multiple operators.
- Most features exposed through the REST API, making it easy to automate things.
- Uses web sockets for faster comms.

**C) User Interface**
- Smooth and reactive UI thanks to *Svelte* and *SocketIO*.
- Easy to configure as it compiles into static HTML, JavaScript, and CSS files, which can be hosted with even the most basic web server you can find.
- Teamchat feature to communicate with other operators over text.

## Installing Striker

Clone the repo;

```bash
$ git clone https://github.com/4g3nt47/Striker.git
$ cd Striker
```

The codebase is divided into 4 independent sections;

### 1. The C2 Server / Backend

This handles all server-side logic for both operators and agents. It is a `NodeJS` application made with;
- `express` - For the REST API.
- `socket.io` - For Web Socket communtication.
- `mongoose` - For connecting to *MongoDB*.
- `multer` - For handling file uploads.
- `bcrypt` - For hashing user passwords.

The source code is in the `backend/` directory. To setup the server;

1. Setup a MongoDB database;

Striker uses MongoDB as backend database to store all important data. You can install this locally on your machine using [this guide](https://www.digitalocean.com/community/tutorials/how-to-install-mongodb-on-ubuntu-20-04) for debian-based distros, or create a free one with [MongoDB Atlas](https://www.freecodecamp.org/news/get-started-with-mongodb-atlas/) (A database-as-a-service platform).

2. Move into the source directory;

```bash
$ cd backend
```

3. Install dependencies;

```bash
$ npm install
```

4. Create a directory for static files;

```bash
$ mkdir static
```

You can use this folder to host static files on the server. This should also be where your `UPLOAD_LOCATION` is set to in the `.env` file (more on this later), but this is not necessary. Files in this directory will be publicly accessible under the path `/static/`.

5. Create a `.env` file;

NOTE: Values between `<` and `>` are placeholders. Replace them with appropriate values (including the `<>`).
For fields that require random strings, you can generate them easily using;
```bash
$ head -c 100 /dev/urandom | sha256sum
```

```text
DB_URL=<your MongoDB connection URL>
HOST=<host to listen on (default: 127.0.0.1)>
PORT=<port to listen on (default: 3000)>
SECRET=<random string to use for signing session cookies and encrypting session data>
ORIGIN_URL=<full URL of the server you will be hosting the frontend at. Used to setup CORS>
REGISTRATION_KEY=<random string to use for authentication during signup>
MAX_UPLOAD_SIZE=<max file upload size, in bytes>
UPLOAD_LOCATION=<directory to store uploaded files to (default: static)>
SSL_KEY=<your SSL key file (optional)>
SSL_CERT=<your SSL cert file (optional)>
```

Note that `SSL_KEY` and `SSL_CERT` are optional. If any is not defined, a plain HTTP server will be created. This helps avoid needless overhead when running the server behind an SSL-enabled reverse proxy on the same host.

6. Start the server;

```bash
$ node index.js
[12:45:30 PM]  Connecting to backend database...
[12:45:31 PM]  Starting HTTP server...
[12:45:31 PM]  Server started on port: 3000
```

### 2. The Frontend

This is the web UI used by operators. It is a single page web application written in *Svelte*, and the source code is in the `frontend/` directory.

To setup the frontend;

1. Move into the source directory;

```bash
$ cd frontend
```

2. Install dependencies;

```bash
$ npm install
```

3. Create a `.env` file with the variable `VITE_STRIKER_API` set to the full URL of the C2 server as configured above;

```bash
VITE_STRIKER_API=https://c2.striker.local
```

4. Build;

```bash
$ npm run build
```

The above will compile everything into a static web application in `dist/` directory. You can move all the files inside into the web root of your web server, or even host it with a basic HTTP server like that of python;

```
$ cd dist
$ python3 -m http.server 8000
```

5. Signup;

- Open the site in a web browser. You should see a login page.
- Click on the `Register` button.
- Enter a username, password, and the registration key in use (see `REGISTRATION_KEY` in `backend/.env`)

This will create a standard user account. You will need an admin account to access some features. 
Your first admin account must be created manually, afterwards you can upgrade and downgrade other accounts in the `Users` tab of the web UI.

To create your first admin account;
- Connect to the MongoDB database used by the backend.
- Update the `users` collection and set the `admin` field of the target user to `true`;

There are different ways you can do this. If you have `mongo` available in you CLI, you can do it using;

```bash
$ mongo <your MongoDB connection URL>
> db.users.updateOne({username: "<your username>"}, {$set: {admin: true}})
```

You should get the following response if it works;

```json
{ "acknowledged" : true, "matchedCount" : 1, "modifiedCount" : 1 }
```

You can now login :)

### 3. The C2 Redirector

**A) Dumb Pipe Redirection**

A dumb pipe redirector written for *Striker* is available at `redirector/redirector.py`. Obviously, this will only work for plain HTTP traffic, or for HTTPS when SSL verification is disabled (you can do this by enabling the `INSECURE_SSL` macro in the C agent).

The following example listens on port `443` on all interfaces and forward to `c2.example.org` on port `443`;

```bash
$ cd redirector
$ ./redirector.py 0.0.0.0:443 c2.example.org:443
[*] Starting redirector on 0.0.0.0:443...
[+] Listening for connections...
```

**B) Nginx Reverse Proxy as Redirector**

1. Install Nginx;

```bash
$ sudo apt install nginx
```

2. Create a vhost config (e.g: `/etc/nginx/sites-available/striker`);

Placeholders;
- `<domain-name>` - This is your server's FQDN, and should match the one in you SSL cert.
- `<ssl-cert>` - The SSL cert file to use.
- `<ssl-key>` - The SSL key file to use.
- `<c2-server>` - The full URL of the C2 server to forward requests to.

WARNING: `client_max_body_size` should be as large as the size defined by `MAX_UPLOAD_SIZE` in your `backend/.env` file, or uploads for large files will fail.

```yaml
server {
    listen 443 ssl;
    server_name             <domain-name>;
    ssl_certificate         <ssl-cert>;
    ssl_certificate_key     <ssl-key>;
    client_max_body_size    100M;
    access_log              /var/log/nginx/striker.log;

    location / {
      proxy_pass              <c2-server>;
      proxy_redirect          off;
      proxy_ssl_verify        off;
      proxy_read_timeout      90;
      proxy_http_version      1.0;
      proxy_set_header        Upgrade $http_upgrade;
      proxy_set_header        Connection "upgrade";
      proxy_set_header        Host $host;
      proxy_set_header        X-Real-IP $remote_addr;
      proxy_set_header        X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```

3. Enable it;

```bash
$ sudo ln -s /etc/nginx/sites-available/striker /etc/nginx/sites-enabled/striker
```

4. Restart Nginx;

```bash
$ sudo service nginx restart
```

Your redirector should now be up and running on port `443`, and can be tested using (assuming your FQDN is `striker.local`);

```bash
$ curl https://striker.local
```

If it works, you should get the 404 response used by the backend, like;

```json
{"error":"Invalid route!"}
```

### 4. The Agents (Implants)

**A) The C Agent**

These are the implants used by Striker. The primary agent is written in C, and is located in `agent/C/`. It supports both linux and windows hosts. The linux agent depends externally on `libcurl`, which you will find installed in most systems.

The windows agent does not have an external dependency. It uses `wininet` for comms, which I believe is available on all windows hosts.

1. **Building for linux**

Assuming you're on a 64 bit host, the following will build for 64 host;

```bash
$ cd agent/C
$ mkdir bin
$ make
```

To build for 32 bit on 64;

```bash
$ sudo apt install gcc-multilib
$ make arch=32
```

The above compiles everything into the `bin/` directory. You will need only two files to generate working implants;

- `bin/stub` - This is the agent stub that will be used as template to generate working implants.
- `bin/builder` - This is what you will use to patch the agent stub to generate working implants.

The builder accepts the following arguments;

```bash
$ ./bin/builder 
[-] Usage: ./bin/builder <url> <auth_key> <delay> <stub> <outfile>
```

Where;

- `<url>` - The server to report to. This should ideally be a redirector, but a direct URL to the server will also work.
- `<auth_key>` -  The authentication key to use when connecting to the C2. You can create this in the *auth keys* tab of the web UI.
- `<delay>` - Delay between each callback, in seconds. This should be at least 2, depending on how noisy you want it to be.
- `<stub>` - The stub file to read, `bin/stub` in this case.
- `<outfile>` - The output filename of the new implant.

Example;

```bash
$ ./bin/builder https://localhost:3000 979a9d5ace15653f8ffa9704611612fc 5 bin/stub bin/striker
[*] Obfuscating strings...
[+] 69 strings obfuscated :)
[*] Finding offsets of our markers...
[+] Offsets:
            URL: 0x0000a2e0
       OBFS Key: 0x0000a280
       Auth Key: 0x0000a2a0
          Delay: 0x0000a260
[*] Patching...
[+] Operation completed!
```

2. **Building for windows**

You will need *MinGW* for this. The following will install the 32 and 64 bit dev windows environment;

```bash
$ sudo apt install mingw-w64
```

Build for 64 bit;

```cmd
$ cd agent/C
$ mdkir bin
$ make target=win
```

To compile for 32 bit;

```cmd
$ make target=win arch=32
```

This will compile everything into the `bin/` directory, and you will have the builder and the stub as `bin\stub.exe` and `bin\builder.exe`, respectively.

**B) The Python Agent**

Striker also comes with a self-contained python 3 agent. This is located at `agent/python/`. Only the most basic features are implemented in this agent. Useful for hosts that can't run the C agent but have python installed.

There are 2 file in this directory;

- `stub.py` - This is the payload stub to pass to the builder.
- `builder.py` - This is what you'll be using to generate an implant.

Usage example:

```bash
$ ./builder.py
[-] Usage: builder.py <url> <auth_key> <delay> <stub> <outfile>
# The following will generate a working payload as `output.py`
$ ./builder.py http://localhost:3000 979a9d5ace15653f8ffa9704611612fc 2 stub.py output.py
[*] Loading agent stub...
[*] Writing configs...
[+] Agent built successfully: output.py
# Run it
$ python3 output.py
```

## Getting Started

After following the above instructions, *Striker* should now be ready for use. Kindly go through [the usage guide](/USAGE.md). Have fun, and happy hacking! :)
