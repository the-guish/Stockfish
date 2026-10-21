# Manguruyu for bot-elo-estimator: Stockfish 19 with the scaccosplicatrice club evaluation in
# place of its network, every weight a UCI option. Copy this file to
# images/engines/manguruyu/Dockerfile there; build_docker.sh builds it with images/ as the
# context, like images/engines/stockfish/Dockerfile, which this follows.
#
#   manguruyu     the branch manguruyu of the-guish/stockfish at the pinned commit.
#
# The build still downloads and embeds Stockfish 19's nets, as the unmodified Makefile does; the
# engine loads them and never evaluates them. Compiles for STOCKFISH_ARCH, so build on the
# machine that will run it.
#
# BASE names the Ubuntu image to build on, for a mirror or a base with a proxy's CA in it.

ARG BASE=ubuntu:26.04

FROM ${BASE} AS build

ARG STOCKFISH_ARCH=x86-64-avx2
# Compiler jobs at once. Empty means one per CPU.
ARG BUILD_JOBS=

# the-guish/stockfish, branch manguruyu: "Manguruyu: every club weight and term switchable
# through UCI options".
ARG MANGURUYU_COMMIT=d7ddb2599b12d4245e938b875ac89aad686350eb

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates curl git g++ make \
    && rm -rf /var/lib/apt/lists/*

COPY common/fetch_commit.sh /usr/local/bin/fetch_commit

RUN fetch_commit https://github.com/the-guish/stockfish "${MANGURUYU_COMMIT}" /src/manguruyu \
    && make -C /src/manguruyu/src -j "${BUILD_JOBS:-$(nproc)}" profile-build ARCH="${STOCKFISH_ARCH}" \
    && strip /src/manguruyu/src/stockfish

RUN mkdir -p /out/manguruyu \
    && cp /src/manguruyu/src/stockfish /src/manguruyu/Copying.txt /out/manguruyu/

FROM scratch
COPY --from=build /out/ /opt/engines/
