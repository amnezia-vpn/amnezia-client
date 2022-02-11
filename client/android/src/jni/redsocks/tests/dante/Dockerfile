FROM ubuntu:14.04

RUN set -o xtrace \
    && sed -i 's,^deb-src,# no src # &,; s,http://archive.ubuntu.com/ubuntu/,mirror://mirrors.ubuntu.com/mirrors.txt,' /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y dante-server

RUN useradd sockusr --user-group \
    --home-dir /nonexistent --no-create-home \
    --shell /usr/sbin/nologin \
    --password '$6$U1HPxoVq$XFqhRetreV3068UCwQA//fGVFUfwfyqeiYpCpeUFAhuMi/wjOhJSzUxb4wUqt9vRnWjO0CDPMkE40ptHWrrIz.'

COPY danted-*.conf /etc/
COPY danted-waitif /usr/sbin/

ENTRYPOINT ["/usr/sbin/danted-waitif", "-d", "-f"]
CMD ["/etc/danted-1080.conf"]
