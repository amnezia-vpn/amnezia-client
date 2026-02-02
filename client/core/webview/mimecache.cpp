#include "mimecache.h"

#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <QtCore/QString>
#include <QtCore/QMimeDatabase>
#include <QtCore/QMimeType>
#include <QtCore/QHash>
#include <QtCore/QDebug>

typedef QHash<QString, QString> MimeTypes;
Q_GLOBAL_STATIC(MimeTypes, cache)
Q_GLOBAL_STATIC(QMimeDatabase, mimeDatabase)


void initCache()
{
	if (cache()->isEmpty()) {

		cache()->insert("323",   "text/h323");
		cache()->insert("*",     "application/octet-stream");
		cache()->insert("acx",   "application/internet-property-stream");
		cache()->insert("ai",    "application/postscript");
		cache()->insert("aif",   "audio/x-aiff");
		cache()->insert("aifc",  "audio/x-aiff");
		cache()->insert("aiff",  "audio/x-aiff");
		cache()->insert("asf",   "video/x-ms-asf");
		cache()->insert("asr",   "video/x-ms-asf");
		cache()->insert("asx",   "video/x-ms-asf");
		cache()->insert("au",    "audio/basic");
		cache()->insert("avi",   "video/x-msvideo");
		cache()->insert("axs",   "application/olescript");
		cache()->insert("bas",   "text/plain");
		cache()->insert("bcpio", "application/x-bcpio");
		cache()->insert("bin",   "application/octet-stream");
		cache()->insert("bmp",   "image/bmp");
		cache()->insert("c",     "text/plain");
		cache()->insert("cat",   "application/vnd.ms-pkiseccat");
		cache()->insert("cdf",   "application/x-cdf");
		cache()->insert("cdf",   "application/x-netcdf");
		cache()->insert("cer",   "application/x-x509-ca-cert""cer");
		cache()->insert("class", "application/octet-stream");
		cache()->insert("clp",   "application/x-msclip");
		cache()->insert("cmx",   "image/x-cmx");
		cache()->insert("cod",   "image/cis-cod");
		cache()->insert("cpio",  "application/x-cpio");
		cache()->insert("crd",   "application/x-mscardfile");
		cache()->insert("crl",   "application/pkix-crl");
		cache()->insert("crt",   "application/x-x509-ca-cert");
		cache()->insert("csh",   "application/x-csh");
		cache()->insert("css",   "text/css");
		cache()->insert("dcr",   "application/x-director");
		cache()->insert("der",   "application/x-x509-ca-cert");
		cache()->insert("dir",   "application/x-director");
		cache()->insert("dll",   "application/x-msdownload");
		cache()->insert("dms",   "application/octet-stream");
		cache()->insert("doc",   "application/msword");
		cache()->insert("dot",   "application/msword");
		cache()->insert("dvi",   "application/x-dvi");
		cache()->insert("dxr",   "application/x-director");
        cache()->insert("eot",     "application/vnd.ms-fontobject");
		cache()->insert("eps",   "application/postscript");
		cache()->insert("etx",   "text/x-setext");
		cache()->insert("evy",   "application/envoy");
		cache()->insert("exe",   "application/octet-stream");
		cache()->insert("fif",   "application/fractals");
		cache()->insert("flr",   "x-world/x-vrml");
		cache()->insert("gif",   "image/gif");
		cache()->insert("gtar",  "application/x-gtar");
		cache()->insert("gz",    "application/x-gzip");
		cache()->insert("h",     "text/plain");
		cache()->insert("hdf",   "application/x-hdf");
		cache()->insert("hlp",   "application/winhlp");
		cache()->insert("hqx",   "application/mac-binhex40");
		cache()->insert("hta",   "application/hta");
		cache()->insert("htc",   "text/x-component");
		cache()->insert("htm",   "text/html");
		cache()->insert("html",  "text/html");
		cache()->insert("htt",   "text/webviewhtml");
		cache()->insert("ico",   "image/x-icon");
		cache()->insert("ief",   "image/ief");
		cache()->insert("iii",   "application/x-iphone");
		cache()->insert("ins",   "application/x-internet-signup");
		cache()->insert("isp",   "application/x-internet-signup");
		cache()->insert("jfif",  "image/pipeg");
		cache()->insert("jpe",   "image/jpeg");
		cache()->insert("jpeg",  "image/jpeg");
		cache()->insert("jpg",   "image/jpeg");
		cache()->insert("js",    "application/x-javascript");
        cache()->insert("json",  "application/json");
		cache()->insert("latex", "application/x-latex");
		cache()->insert("lha",   "application/octet-stream");
		cache()->insert("lsf",   "video/x-la-asf");
		cache()->insert("lsx",   "video/x-la-asf");
		cache()->insert("lzh",   "application/octet-stream");
		cache()->insert("m13",   "application/x-msmediaview");
		cache()->insert("m14",   "application/x-msmediaview");
		cache()->insert("m3u",   "audio/x-mpegurl");
		cache()->insert("m4v",   "video/x-m4v");
		cache()->insert("man",   "application/x-troff-man");
		cache()->insert("mdb",   "application/x-msaccess");
		cache()->insert("me",    "application/x-troff-me");
		cache()->insert("mht",   "message/rfc822");
		cache()->insert("mhtml", "message/rfc822");
		cache()->insert("mid",   "audio/mid");
		cache()->insert("mny",   "application/x-msmoney");
		cache()->insert("mov",   "video/quicktime");
		cache()->insert("movie", "video/x-sgi-movie""movie");
		cache()->insert("mp2",   "video/mpeg");
		cache()->insert("mp3",   "audio/mpeg");
		cache()->insert("mpa",   "video/mpeg");
		cache()->insert("mpe",   "video/mpeg");
		cache()->insert("mpeg",  "video/mpeg");
		cache()->insert("mpg",   "video/mpeg");
		cache()->insert("mpp",   "application/vnd.ms-project");
		cache()->insert("mpv2",  "video/mpeg");
		cache()->insert("ms",    "application/x-troff-ms");
		cache()->insert("msg",   "application/vnd.ms-outlook");
		cache()->insert("mvb",   "application/x-msmediaview");
		cache()->insert("nc",    "application/x-netcdf");
		cache()->insert("nws",   "message/rfc822");
		cache()->insert("oda",   "application/oda");
        cache()->insert("otf",   "application/font-sfnt");
		cache()->insert("p10",   "application/pkcs10");
		cache()->insert("p12",   "application/x-pkcs12");
		cache()->insert("p7b",   "application/x-pkcs7-certificates");
		cache()->insert("p7c",   "application/x-pkcs7-mime");
		cache()->insert("p7m",   "application/x-pkcs7-mime");
		cache()->insert("p7r",   "application/x-pkcs7-certreqresp");
		cache()->insert("p7s",   "application/x-pkcs7-signature");
		cache()->insert("pbm",   "image/x-portable-bitmap");
		cache()->insert("pdf",   "application/pdf");
		cache()->insert("pfx",   "application/x-pkcs12");
		cache()->insert("pgm",   "image/x-portable-graymap");
		cache()->insert("pko",   "application/ynd.ms-pkipko");
		cache()->insert("pma",   "application/x-perfmon");
		cache()->insert("pmc",   "application/x-perfmon");
		cache()->insert("pml",   "application/x-perfmon");
		cache()->insert("pmr",   "application/x-perfmon");
		cache()->insert("pmw",   "application/x-perfmon");
		cache()->insert("pnm",   "image/x-portable-anymap");
		cache()->insert("pot",   "application/vnd.ms-powerpoint");
		cache()->insert("ppm",   "image/x-portable-pixmap");
		cache()->insert("pps",   "application/vnd.ms-powerpoint");
		cache()->insert("ppt",   "application/vnd.ms-powerpoint");
		cache()->insert("prf",   "application/pics-rules");
		cache()->insert("ps",    "application/postscript");
		cache()->insert("pub",   "application/x-mspublisher");
		cache()->insert("qt",    "video/quicktime");
		cache()->insert("ra",    "audio/x-pn-realaudio");
		cache()->insert("ram",   "audio/x-pn-realaudio");
		cache()->insert("ras",   "image/x-cmu-raster");
		cache()->insert("rgb",   "image/x-rgb");
		cache()->insert("rmi",   "audio/mid");
		cache()->insert("roff",  "application/x-troff");
		cache()->insert("rtf",   "application/rtf""rtf");
		cache()->insert("rtx",   "text/richtext""rtx");
		cache()->insert("scd",   "application/x-msschedule");
		cache()->insert("sct",   "text/scriptlet");
		cache()->insert("setpay", "application/set-payment-initiation");
		cache()->insert("setreg", "application/set-registration-initiation");
		cache()->insert("sh",     "application/x-sh");
		cache()->insert("shar",   "application/x-shar");
		cache()->insert("sit",    "application/x-stuffit");
		cache()->insert("snd",    "audio/basic");
		cache()->insert("spc",    "application/x-pkcs7-certificates");
		cache()->insert("spl",    "application/futuresplash");
		cache()->insert("src",    "application/x-wais-source");
		cache()->insert("sst",    "application/vnd.ms-pkicertstore");
		cache()->insert("stl",    "application/vnd.ms-pkistl");
		cache()->insert("stm",    "text/html");
		cache()->insert("sv4cpio", "application/x-sv4cpio");
		cache()->insert("sv4crc",  "application/x-sv4crc");
		cache()->insert("svg",     "image/svg+xml");
		cache()->insert("swf",     "application/x-shockwave-flash");
		cache()->insert("t",       "application/x-troff");
		cache()->insert("tar",     "application/x-tar");
		cache()->insert("tcl",     "application/x-tcl");
		cache()->insert("tex",     "application/x-tex");
		cache()->insert("texi",    "application/x-texinfo");
		cache()->insert("texinfo", "application/x-texinfo");
		cache()->insert("tgz",     "application/x-compressed");
		cache()->insert("tif",     "image/tiff");
		cache()->insert("tiff",    "image/tiff");
		cache()->insert("tr",      "application/x-troff");
		cache()->insert("trm",     "application/x-msterminal");
		cache()->insert("tsv",     "text/tab-separated-values");
		cache()->insert("txt",     "text/plain");
        cache()->insert("ttf",     "application/font-sfnt");
        cache()->insert("uls",     "text/iuls");
		cache()->insert("ustar",   "application/x-ustar");
		cache()->insert("vcf",     "text/x-vcard");
		cache()->insert("vrml",    "x-world/x-vrml");
		cache()->insert("wav",     "audio/x-wav");
		cache()->insert("wcm",     "application/vnd.ms-works");
		cache()->insert("wdb",     "application/vnd.ms-works");
		cache()->insert("wks",     "application/vnd.ms-works");
		cache()->insert("wmf",     "application/x-msmetafile");
        cache()->insert("woff",    "application/font-woff");
		cache()->insert("wps",     "application/vnd.ms-works");
		cache()->insert("wri",     "application/x-mswrite");
		cache()->insert("wrl",     "x-world/x-vrml");
		cache()->insert("wrz",     "x-world/x-vrml");
		cache()->insert("xaf",     "x-world/x-vrml");
		cache()->insert("xbm",     "image/x-xbitmap");
		cache()->insert("xla",     "application/vnd.ms-excel");
		cache()->insert("xlc",     "application/vnd.ms-excel");
		cache()->insert("xlm",     "application/vnd.ms-excel");
		cache()->insert("xls",     "application/vnd.ms-excel");
		cache()->insert("xlt",     "application/vnd.ms-excel");
		cache()->insert("xlw",     "application/vnd.ms-excel");
		cache()->insert("xof",     "x-world/x-vrml");
		cache()->insert("xpm",     "image/x-xpixmap");
		cache()->insert("xwd",     "image/x-xwindowdump");
		cache()->insert("z",       "application/x-compress");
		cache()->insert("zip",     "application/zip");
	}
}

QString mimeTypeForExtension(const QString &extension)
{
    initCache();
    QString ext = extension;

    const int lastDot = ext.lastIndexOf(QLatin1Char('.'));
    if (lastDot != -1) {
    	const int extLength = ext.length() - lastDot - 1;
    	ext = ext.right(extLength).toLower();
    }

    QString mimeType = cache()->value(ext);

	if (mimeType.isNull()) {
		mimeType = QString("application/octet-stream");
    }

    return mimeType;
}

QString mimeTypeForUrl(const QUrl &url)
{
	initCache();
	QString path = url.path();

	QString extension;
	QString mimeType;

	const int lastDot = path.lastIndexOf(QLatin1Char('.'));
	if (lastDot != -1) {
		const int extLength = path.length() - lastDot - 1;
		extension = path.right(extLength).toLower();
	}

	if (!extension.isNull()) {
		mimeType = cache()->value(extension);
	}
	if (mimeType.isNull()) {
		QMimeType mime = mimeDatabase()->mimeTypeForUrl(url);
		if (mime.isValid()) {
			mimeType = mime.name();
			if(!extension.isNull()) {
				cache()->insert(extension, mimeType);
			}
		}
	}

	if (mimeType.isNull()) {
		mimeType = QString("application/octet-stream");
	}

	qDebug() << mimeType;
	return mimeType;
}

