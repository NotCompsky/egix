FROM ubuntu:16.04
COPY . /egix
RUN apt update \
	&& apt install -y --no-install-recommends \
		libboost-regex-dev \
		meson \
		python3-setuptools \
		python3-mako \
		pkg-config \
		libdrm-intel1 \
		libbsd-dev \
	\
	&& git clone --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git \
	&& cd mesa \
	&& sed -i 's/libgl = shared_library/libgl = static_library/g' src/glx/meson.build \
	&& meson builddir/ \
	\
	&& cd / \
	&& git clone --depth 1 https://github.com/NotCompsky/libcompsky \
	&& mkdir libcompsky/build \
	&& cmake \
		-DCMAKE_BUILD_TYPE=Release \
		.. \
	&& cd libcompsky/build \
	&& make install \
	\
	&& cd /egix \
	&& cmake \
		-DCMAKE_BUILD_TYPE=Release \
		-DENABLE_STATIC=OFF \
		-DBUILD_PROGRAM=ON \
		/egix \
	&& make \
	&& strip --strip-all egixr \
	&& curl -sL https://github.com/probonopd/linuxdeployqt/releases/download/7/linuxdeployqt-7-x86_64.AppImage > linuxdeployqt \
	&& chmod a+x linuxdeployqt \
	&& ./linuxdeployqt \
		egixr \
		-appimage \
		-unsupported-bundle-everything \
		-no-copy-copyright-files \
		-no-translations

FROM alpine:latest
COPY --from=intermediate /egixr/egix /egix
ENTRYPOINT /bin/sh
