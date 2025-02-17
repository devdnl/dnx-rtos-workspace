FROM opensuse/tumbleweed

RUN zypper --non-interactive in opi
RUN zypper --non-interactive in make makedepend gcc
RUN zypper --non-interactive in wget
RUN zypper --non-interactive in tar
RUN zypper --non-interactive in bzip2
RUN zypper --non-interactive in python3
RUN zypper --non-interactive in bash-completion
RUN zypper --non-interactive in zsh
RUN zypper --non-interactive in git
RUN zypper --non-interactive in libncurses5 libncurses6 ncurses-devel
RUN zypper --non-interactive in libreadline6 libreadline8
RUN zypper --non-interactive in libgtk-2_0-0
RUN zypper --non-interactive in libgthread-2_0-0
RUN zypper --non-interactive in libSM6
RUN zypper --non-interactive in libGLU1
RUN zypper --non-interactive in srecord
RUN zypper --non-interactive in minicom minicom-lang
RUN zypper --non-interactive in which
RUN zypper --non-interactive in libtool
RUN zypper --non-interactive in pkgconf-pkg-config
RUN zypper --non-interactive in jimtcl jimtcl-devel
RUN zypper --non-interactive in libusb-1_0-0 libusb-1_0-devel
RUN zypper --non-interactive in libjaylink libjaylink-devel libjaylink0
RUN zypper --non-interactive in gawk lua53 lua53-devel lua53-luarocks

ARG ARCHITECTURE=

ENV HOME=/home/developer

WORKDIR /home/developer

# basic files and directories
RUN echo 'export PATH=$PATH:/home/developer/bin' > /home/developer/.bashrc
RUN useradd --no-log-init --create-home --shell /bin/zsh developer
RUN mkdir -p /home/developer/bin
RUN mkdir -p /home/developer/project

# arm-none-eabi-gcc
RUN wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-${ARCHITECTURE}-linux.tar.bz2
RUN tar -jxf gcc-arm-none-eabi-10.3-2021.10-${ARCHITECTURE}-linux.tar.bz2 -C /home/developer/bin/
RUN rm gcc-arm-none-eabi-10.3-2021.10-${ARCHITECTURE}-linux.tar.bz2
RUN echo 'export PATH=$PATH:/home/developer/bin/gcc-arm-none-eabi-10.3-2021.10/bin' >> /home/developer/.bashrc

# oh-my-zsh
RUN ZSH=/home/developer/.oh-my-zsh sh -c "$(wget -O- https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
RUN echo 'export ZSH="/home/developer/.oh-my-zsh"' > .zshrc
RUN echo 'ZSH_THEME="fwalch"' >> .zshrc
RUN echo 'plugins=(git)' >> .zshrc
RUN echo 'source $ZSH/oh-my-zsh.sh' >> .zshrc
RUN echo 'source $HOME/.bashrc' >> .zshrc
CMD ["zsh", "-l"]

# openocd compilation
WORKDIR /home/developer/bin
RUN git clone https://git.code.sf.net/p/openocd/code openocd-code
WORKDIR /home/developer/bin/openocd-code
RUN chmod +x ./bootstrap
RUN ./bootstrap
RUN ./configure --enable-ftdi --enable-jlink --enable-stlink --enable-picoprobe
RUN make -j all
RUN mkdir -p /home/developer/bin/openOCD
RUN cp /home/developer/bin/openocd-code/src/openocd /home/developer/bin/openOCD/openocd
RUN cp -R /home/developer/bin/openocd-code/tcl/* /home/developer/bin/openOCD/.
WORKDIR /home/developer/bin
RUN rm -rf /home/developer/bin/openocd-code
RUN echo '#!/usr/bin/env bash' > /home/developer/bin/openocd
RUN echo 'cd $(dirname $0)' >> /home/developer/bin/openocd
RUN echo 'cd ./openOCD' >> /home/developer/bin/openocd
RUN echo './openocd "$@"' >> /home/developer/bin/openocd
RUN chmod +x /home/developer/bin/openocd

# lua
RUN luarocks install lcurses

RUN chown -R developer:users /home/developer
