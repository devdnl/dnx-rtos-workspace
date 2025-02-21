FROM opensuse/leap:15.6

ARG ARCHITECTURE=
ARG PROJECT_NAME=

# packages
ARG ZYPPER="zypper --non-interactive"
RUN ${ZYPPER} in opi glibc-locale
RUN ${ZYPPER} in make makedepend gcc
RUN ${ZYPPER} in wget
RUN ${ZYPPER} in tar
RUN ${ZYPPER} in bzip2
RUN ${ZYPPER} in python3
RUN ${ZYPPER} in bash-completion
RUN ${ZYPPER} in zsh
RUN ${ZYPPER} in git
RUN ${ZYPPER} in libncurses5 libncurses6 ncurses-devel
RUN ${ZYPPER} in libreadline6 libreadline7
RUN ${ZYPPER} in libgtk-2_0-0
RUN ${ZYPPER} in libgthread-2_0-0
RUN ${ZYPPER} in libSM6
RUN ${ZYPPER} in libGLU1
RUN ${ZYPPER} in srecord
RUN ${ZYPPER} in minicom minicom-lang
RUN ${ZYPPER} in which
RUN ${ZYPPER} in libtool
RUN ${ZYPPER} in pkgconf
RUN ${ZYPPER} in jimtcl jimtcl-devel
RUN ${ZYPPER} in libusb-1_0-0 libusb-1_0-devel
RUN ${ZYPPER} in libjaylink libjaylink-devel libjaylink0
RUN ${ZYPPER} in gawk lua53 lua53-devel lua53-luarocks
RUN ${ZYPPER} in xorg-x11-fonts xorg-x11-fonts-converted xorg-x11-fonts-core xorg-x11-fonts-legacy

# post-install operations
RUN ln -s $(which srec_cat) /bin/srec_cat

# environment variables
ENV HOME=/home/developer
ENV LANG="C.UTF-8"
ENV LC_ALL="C.UTF-8"
ENV DISPLAY=:0

# basic files and directories
WORKDIR /home/developer
RUN echo 'export PATH=$PATH:/home/developer/bin' > /home/developer/.bashrc
RUN useradd --no-log-init --create-home --shell /bin/zsh developer
RUN mkdir -p /home/developer/bin
RUN mkdir -p /home/developer/${PROJECT_NAME}

# arm-none-eabi-gcc
WORKDIR /home/developer
RUN wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-${ARCHITECTURE}-linux.tar.bz2
RUN tar -jxf gcc-arm-none-eabi-10.3-2021.10-${ARCHITECTURE}-linux.tar.bz2 -C /home/developer/bin/
RUN rm gcc-arm-none-eabi-10.3-2021.10-${ARCHITECTURE}-linux.tar.bz2
RUN echo 'export PATH=$PATH:/home/developer/bin/gcc-arm-none-eabi-10.3-2021.10/bin' >> /home/developer/.bashrc

#RUN wget https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-${ARCHITECTURE}-arm-none-eabi.tar.xz
#RUN tar -xf arm-gnu-toolchain-14.2.rel1-${ARCHITECTURE}-arm-none-eabi.tar.xz -C /home/developer/bin/
#RUN rm arm-gnu-toolchain-14.2.rel1-${ARCHITECTURE}-arm-none-eabi.tar.xz
#RUN echo "export PATH=\$PATH:/home/developer/bin/arm-gnu-toolchain-14.2.rel1-${ARCHITECTURE}-arm-none-eabi/bin" >> /home/developer/.bashrc

# oh-my-zsh
WORKDIR /home/developer
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

# set user id and group
RUN chown -R developer:users /home/developer
RUN echo 'plugdev:x:1001:developer' >> /etc/group
