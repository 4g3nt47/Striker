# Redirector

This is a simple dumb pipe redirector for routing **unencrypted C2 traffic** for Striker. Except for local testing, you should never use this alone as a redirector. It should only be used between a HTTPs enabled reverse proxy like Nginx to forward unencrypted traffic to the C2 server over plain HTTP, or another dumb pipe redirector like socat.

### Using Nginx Reverse Proxy as Redirector

1. Install `nginx`;

```bash
$ sudo apt install nginx
```

2. Create a vhost config (e.g: `/etc/nginx/sites-available/striker`);

Placeholders;
- `<domain-name>` - This is the FQDN in your SSL cert.
- `<ssl-cert>` - The SSL cert file to use.
- `<ssl-key>` - The SSL key file to use.
- `<c2-server>` - The full URL of the C2 server to forward requests to.

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

4. Restart nginx;

```bash
$ sudo service nginx restart
```

Your redirector should now be up and running on port `443`, and can be tested using (assuming your FQDN is `striker.local`);

```bash
$ curl https://striker.local
```
