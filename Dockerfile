FROM cyclus/cycamore

COPY . /mbmore
WORKDIR /mbmore
RUN ./install.py --prefix="/usr/local"

COPY cloudlus /usr/local/bin/cloudlus
ENTRYPOINT ["cloudlus", "-addr", "dory.fuelcycle.org:1441", "work", "-interval", "3s", "-whitelist", "cyclus", "cyan", "-timeout=3m"]
