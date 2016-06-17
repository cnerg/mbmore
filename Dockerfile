FROM cyclus/cyclus:latest

RUN git clone https://github.com/cyclus/cycamore.git /cycamore
WORKDIR /cycamore
RUN git fetch
RUN git checkout develop
RUN ./install.py --prefix="/usr/local"

COPY . /mbmore
WORKDIR /mbmore
RUN ./install.py --prefix="/usr/local"

COPY cloudlus /usr/local/bin/cloudlus
ENTRYPOINT ["cloudlus", "-addr", "dory.fuelcycle.org:1441", "work", "-interval", "3s", "-whitelist", "cyclus", "cyan", "-timeout=3m"]
