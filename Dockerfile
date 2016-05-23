FROM cyclus/cycamore

COPY . /mbmore
WORKDIR /mbmore
RUN ./install.py

COPY cloudlus /usr/local/bin/cloudlus
ENTRYPOINT ["cloudlus", "-addr", "127.0.0.1:1441", "work", "-interval", "3s", "-whitelist", "cyclus", "cyan", "-timeout=3m"]
