FROM ubuntu:24.04

SHELL ["/bin/bash", "-c"]

RUN apt update && \
    apt install -y \
        git \
        cmake \
        build-essential \
        python3 \
        python3-dev \
        python3-pip \
        python3-virtualenv && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /compiler-research

RUN mkdir -p env-setup

COPY clone.sh env-setup/clone.sh
COPY setup.sh env-setup/setup.sh
COPY env.sh env-setup/env.sh

RUN chmod +x env-setup/*.sh

RUN ./env-setup/clone.sh
RUN ./env-setup/setup.sh
RUN ./env-setup/env.sh

RUN echo 'source /compiler-research/env-setup/env.sh' >> /etc/bash.bashrc

CMD ["bash"]