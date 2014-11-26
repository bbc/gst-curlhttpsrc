# GstCurlHttpSrc

**GstCurlHttpSrc** is a GStreamer URIHandler plugin that supports getting
resources from web servers using HTTP/HTTPS. It uses the curl library to
acheive this, and supports HTTP/1.0, HTTP/1.1 and HTTP/2.0 protocols. (Actual
support is dependent on curl version and configuration)

**GstCurlHttpSrc** implements the URIHandler interface for http:// and https://
URIs, meaning as long as other gstreamer elements or applications use the
URIHandler interface, they need not be changed to support this module as
opposed to using any others.

## Getting Started

**GstCurlHttpSrc** requires [GStreamer](http://gstreamer.freedesktop.org/) 1.x
and any modern version of libcurl. The curl library used may be optionally
compiled against an SSL library such as [OpenSSL](http://www.openssl.org) for
https:// support; and against [nghttp2](http://www.nghttp2.org) for HTTP/2
support.

The software has been developed on Linux Mint 17 (Ubuntu 14.04-alike). This
README has two parts; the **Quick** install and the **Full** install. The Quick
install assumes that you already have a working version of cURL and GStreamer
installed on your machine, in a sensible location. The Full install walks you
through building GStreamer, curl, OpenSSL and nghttp2 from scratch.

In effect, use the Quick guide to just build this module, use the Full guide
if you want everything.

### Quick Guide

#### Download the source

    $ git clone https://github.com/BBC/gst-curlhttpsrc.git

#### Build the source

    $ cd gst-curlhttpsrc
    $ ./autogen.sh
    $ make
    $ make install

Beware that if your GStreamer is installed in `/usr` or somewhere similar, you
will need to be root in order to perform the last command.

#### Run the application

In order to just fetch one file and then dump to the console, you can do the
following:

    $ gst-launch-1.0 curlhttpsrc location=http://example.com/ ! fakesink dump=1

The following assumes that you have a build of GStreamer including all the
necessary plugins in order to play an MPEG-DASH stream. The MPD file in
question has AVC3 video and AAC audio, and comes from
[here](http://rdmedia.bbc.co.uk/dash/ondemand/bbb/). Full details of all
representations and additional MPDs can be found at that link too.

    $ gst-launch-1.0 playbin uri=http://rdmedia.bbc.co.uk/dash/ondemand/bbb/avc3/1/client_manifest-common_init.mpd

#### Default URIHandler

By default, GStreamer uses SoupHTTPSrc as it's URIHandler implementation for
http and https URIs. In order to change back to using SoupHTTPSrc, you can
either remove the CurlHttpSrc modules from `/usr/lib/gstreamer-1.0` or you can
change the element rank so that SoupHTTPSrc takes precidence once again. This
value can be found at the bottom of the gstcurlhttpsrc.c file. Simply change
the line that looks like this:

```
  return gst_element_register (curlhttpsrc, "curlhttpsrc", 500,
      GST_TYPE_CURLHTTPSRC);
```

To this:

```
  return gst_element_register (curlhttpsrc, "curlhttpsrc", 0,
      GST_TYPE_CURLHTTPSRC);
```

### Full Guide

#### Required software

| Package | Project Website | Download |
| ------- | --------------- | -------- |
| GStreamer | http://gstreamer.freedesktop.org/ | [1.4.3 Release](http://gstreamer.freedesktop.org/releases/gstreamer/1.4.3.html) |
| GStreamer Plugins Base | http://gstreamer.freedesktop.org/ | [1.4.3 Release](http://gstreamer.freedesktop.org/releases/gst-plugins-base/1.4.3.html) |
| GStreamer Plugins Good | http://gstreamer.freedesktop.org/ | [1.4.3 Release](http://gstreamer.freedesktop.org/releases/gst-plugins-good/1.4.3.html) |
| Gstreamer Plugins Bad | http://gstreamer.freedesktop.org/ | [1.4.3 Release](http://gstreamer.freedesktop.org/releases/gst-plugins-bad/1.4.3.html) |
| OpenSSL (Optional, for SSL support. Must be 1.1.0 and above for HTTP/2 support) | http://www.openssl.org | git clone https://github.com/openssl/openssl.git |
| nghttp2 (Optional, for HTTP/2 support) | http://nghttp2.org | git clone https://github.com/tatsuhiro-t/nghttp2.git |
| cURL | http://curl.haxx.se | git clone https://github.com/bagder/curl.git |

#### Build preface

It is important to know what you want to do with this software. The intended
usage of this module (at least for the time being) is to show HTTP/2 working.
This requires the usage of some pretty bleeding-edge software that may break,
and as such I wouldn't recommend using it day-to-day or replacing your existing
distribution-supplied versions unless you really want to.

The way I have built this software is to install it in a custom directory in my
home folder, `~/build/target`. The `BUILD_PREFIX` environment variable points
to that directory, with my path being set as `$BUILD_PREFIX/bin:$PATH`, my
`LD_LIBRARY_PATH` variable set as `$BUILD_PREFIX/lib:$PATH` and my
`PKG_CONFIG_PATH` variable set as `$BUILD_PREFIX/lib/pkgconfig`.

If you're fine to replace your existing software (or you don't have any
Distribution supplied ones anyway), then you can ignore any `--prefix` or
`--exec-prefix` options supplied in the build instructions below.

These are all external projects, and therefore the following instructions are
just a guide. The projects are under no control by myself or the BBC and as
such we take no responsibility for the following instructions being correct
as and when you get hold of the software in question.

#### Build GStreamer

GStreamer itself comes in four parts:

* GStreamer Core
    * Contains the core GStreamer framework, as well as top-level applications
such as gst-launch, gst-inspect and playbin.
* GStreamer-Plugins-Base
    * Contains the necessary frameworks to allow loadable plugins
* GStreamer-Plugins-Good
    * Contains a selection of plugins such as SoupHTTPSrc
* GStreamer-Plugins-Bad
    * Contains a selection of plugins with runnable code but lacking something to
stop it from being counted as a "good" plugin, such as:
        * Needs a good code review.
        * Documentation needs improving.
        * No active maintainer.
        * Not being used in a widespread manner.

You'll need to build them in order to minimise any dependency issues.
Thankfully, the build process is the same between each part.

    $ cd gstreamer-1.4.3
    $ ./configure --prefix=$BUILD_PREFIX
    $ make && make install
    $ cd ../gst-plugins-base-1.4.3
    $ ./configure --prefix=$BUILD_PREFIX
    $ make && make install
    $ cd ../gst-plugins-good-1.4.3
    $ # And so on...

#### Build OpenSSL (Optional)

    $ cd openssl
    $ ./config --prefix=$BUILD_PREFIX
    $ make depend
    $ make
    $ make test
    $ make install

#### Build nghttp2 (Optional)

    $ autoreconf -i
    $ ./configure --prefix=$BUILD_PREFIX --exec-prefix=$BUILD_PREFIX
    $ make
    $ make install

#### Build libcurl

    $ ./configure --prefix=$BUILD_PREFIX
    $ make
    $ make install

#### Build CurlHttpSrc

See the Quick Guide section.

## Credits

This plugin contains code derived from the [gst-template](http://cgit.freedesktop.org/gstreamer/gst-template/)
as provided by the GStreamer project.

## License

See the LICENSE file.

## Contributing

If you have a feature request or want to report a bug, we'd be happy to hear
from you. Please either [raise an issue](https://github.com/BBC/gst-curlhttpsrc/issues),
or fork the project and send us a pull request.

## Authors

This software was written by [Sam Hurst](https://github.com/samhurst),
samuelh <at> rd.bbc.co.uk

## Copyright

Copyright 2014 British Broadcasting Corporation

