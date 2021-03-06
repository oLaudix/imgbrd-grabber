#include "image-downloader-test.h"
#include <QtTest>
#include "custom-network-access-manager.h"
#include "downloader/image-downloader.h"
#include "models/filtering/blacklist.h"
#include "models/image.h"
#include "models/profile.h"
#include "models/site.h"
#include "models/source.h"


void ImageDownloaderTest::init()
{
	QDir("tests/resources/").mkdir("tmp");

	setupSource("Danbooru (2.0)");
	setupSite("Danbooru (2.0)", "danbooru.donmai.us");

	m_profile = makeProfile();
	m_site = m_profile->getSites().value("danbooru.donmai.us");
}

void ImageDownloaderTest::cleanup()
{
	QDir dir("tests/resources/tmp/");
	for (const QString &file : dir.entryList(QDir::Files)) {
		dir.remove(file);
	}

	delete m_profile;
	m_profile = nullptr;
}

Image *ImageDownloaderTest::createImage(bool noMd5)
{
	QMap<QString, QString> details;
	if (!noMd5) {
		details["md5"] = "1bc29b36f623ba82aaf6724fd3b16718";
	}
	details["ext"] = "jpg";
	details["id"] = "7331";
	details["file_url"] = "http://test.com/img/oldfilename.jpg";
	details["sample_url"] = "http://test.com/sample/oldfilename.jpg";
	details["preview_url"] = "http://test.com/preview/oldfilename.jpg";
	details["page_url"] = "/posts/7331";
	details["tags"] = "tag1 tag2 tag3";

	return new Image(m_site, details, m_profile);
}


void ImageDownloaderTest::testSuccessBasic()
{
	QSharedPointer<Image> img(createImage());
	ImageDownloader downloader(m_profile, img, "out.jpg", "tests/resources/tmp", 1, false, false, nullptr, false, false);

	QList<ImageSaveResult> expected;
	expected.append({ QDir::toNativeSeparators("tests/resources/tmp/out.jpg"), Image::Size::Full, Image::SaveResult::Saved });

	assertDownload(img, &downloader, expected, true);
}

void ImageDownloaderTest::testSuccessLoadTags()
{
	QSharedPointer<Image> img(createImage());
	ImageDownloader downloader(m_profile, img, "%copyright%.%ext%", "tests/resources/tmp", 1, false, false, nullptr, true, false);

	QList<ImageSaveResult> expected;
	expected.append({ QDir::toNativeSeparators("tests/resources/tmp/to heart 2.jpg"), Image::Size::Full, Image::SaveResult::Saved });

	assertDownload(img, &downloader, expected, true);
}

void ImageDownloaderTest::testSuccessLoadTagsExternal()
{
	QSharedPointer<Image> img(createImage());
	ImageDownloader downloader(m_profile, img, "out.jpg", "tests/resources/tmp", 1, false, false, nullptr, true, false);

	// Delete already existing
	QFile logFile("tests/resources/tmp/savelog.txt");
	if (logFile.exists()) {
		logFile.remove();
	}

	QSettings *settings = m_profile->getSettings();
	settings->setValue("LogFiles/0/locationType", 1);
	settings->setValue("LogFiles/0/uniquePath", logFile.fileName());
	settings->setValue("LogFiles/0/content", "%copyright%");

	QList<ImageSaveResult> expected;
	expected.append({ QDir::toNativeSeparators("tests/resources/tmp/out.jpg"), Image::Size::Full, Image::SaveResult::Saved });

	assertDownload(img, &downloader, expected, true);

	QCOMPARE(logFile.exists(), true);
	QVERIFY2(logFile.open(QFile::ReadOnly | QFile::Text), "Could not open text file");
	QCOMPARE(QString(logFile.readAll()), QString("to heart 2"));

	logFile.close();
	logFile.remove();

	settings->remove("LogFiles/0/locationType");
	settings->remove("LogFiles/0/uniquePath");
	settings->remove("LogFiles/0/content");
}

void ImageDownloaderTest::testSuccessLoadSize()
{
	QSharedPointer<Image> img(createImage());
	ImageDownloader downloader(m_profile, img, "%copyright%.%ext%", "tests/resources/tmp", 1, false, false, nullptr, true, false);

	QList<ImageSaveResult> expected;
	expected.append({ QDir::toNativeSeparators("tests/resources/tmp/to heart 2.jpg"), Image::Size::Full, Image::SaveResult::Saved });

	QVERIFY(img->size().isEmpty());
	assertDownload(img, &downloader, expected, true);
	QCOMPARE(img->size(), QSize(1, 1));
}

void ImageDownloaderTest::testOpenError()
{
	QSharedPointer<Image> img(createImage());
	ImageDownloader downloader(m_profile, img, "///", "///root/toto", 1, false, false, nullptr, false, false);

	QList<ImageSaveResult> expected;
	expected.append({ QDir::toNativeSeparators("//root/toto/"), Image::Size::Full, Image::SaveResult::Error });

	assertDownload(img, &downloader, expected, false, true);
}

void ImageDownloaderTest::testNotFound()
{
	QSharedPointer<Image> img(createImage());
	ImageDownloader downloader(m_profile, img, "out.jpg", "tests/resources/tmp", 1, false, false, nullptr, false, false);

	QList<ImageSaveResult> expected;
	expected.append({ QDir::toNativeSeparators("tests/resources/tmp/out.jpg"), Image::Size::Full, Image::SaveResult::NotFound });

	CustomNetworkAccessManager::NextFiles.append("404");

	assertDownload(img, &downloader, expected, false);
}

void ImageDownloaderTest::testNetworkError()
{
	QSharedPointer<Image> img(createImage());
	ImageDownloader downloader(m_profile, img, "out.jpg", "tests/resources/tmp", 1, false, false, nullptr, false, false);

	QList<ImageSaveResult> expected;
	expected.append({ QDir::toNativeSeparators("tests/resources/tmp/out.jpg"), Image::Size::Full, Image::SaveResult::NetworkError });

	CustomNetworkAccessManager::NextFiles.append("500");

	assertDownload(img, &downloader, expected, false);
}

void ImageDownloaderTest::testOriginalMd5()
{
	QSharedPointer<Image> img(createImage());
	ImageDownloader downloader(m_profile, img, "%md5%.%ext%", "tests/resources/tmp", 1, false, false, nullptr, false, false);

	QList<ImageSaveResult> expected;
	expected.append({ QDir::toNativeSeparators("tests/resources/tmp/1bc29b36f623ba82aaf6724fd3b16718.jpg"), Image::Size::Full, Image::SaveResult::Saved });

	assertDownload(img, &downloader, expected, true);
}

void ImageDownloaderTest::testGeneratedMd5()
{
	QSharedPointer<Image> img(createImage(true));
	ImageDownloader downloader(m_profile, img, "%md5%.%ext%", "tests/resources/tmp", 1, false, false, nullptr, false, false);

	QList<ImageSaveResult> expected;
	expected.append({ QDir::toNativeSeparators("tests/resources/tmp/956ddde86fb5ce85218b21e2f49e5c50.jpg"), Image::Size::Full, Image::SaveResult::Saved });

	assertDownload(img, &downloader, expected, true);
}

void ImageDownloaderTest::testRotateExtension()
{
	QSharedPointer<Image> img(createImage());
	ImageDownloader downloader(m_profile, img, "%md5%.%ext%", "tests/resources/tmp", 1, false, false, nullptr, false, true);

	QList<ImageSaveResult> expected;
	expected.append({ QDir::toNativeSeparators("tests/resources/tmp/1bc29b36f623ba82aaf6724fd3b16718.png"), Image::Size::Full, Image::SaveResult::Saved });

	CustomNetworkAccessManager::NextFiles.append("404");

	assertDownload(img, &downloader, expected, true);
}

void ImageDownloaderTest::testSampleFallback()
{
	QSharedPointer<Image> img(createImage());
	ImageDownloader downloader(m_profile, img, "%md5%.%ext%", "tests/resources/tmp", 1, false, false, nullptr, false, false);

	QList<ImageSaveResult> expected;
	expected.append({ QDir::toNativeSeparators("tests/resources/tmp/1bc29b36f623ba82aaf6724fd3b16718.jpg"), Image::Size::Sample, Image::SaveResult::Saved });

	CustomNetworkAccessManager::NextFiles.append("404");

	assertDownload(img, &downloader, expected, true, false, true);
}

void ImageDownloaderTest::testBlacklisted()
{
	Blacklist blacklist(QStringList() << "tag1");

	QSharedPointer<Image> img(createImage());
	ImageDownloader downloader(m_profile, img, "out.jpg", "tests/resources/tmp", 1, false, false, nullptr, false, false);
	downloader.setBlacklist(&blacklist);

	QList<ImageSaveResult> expected;
	expected.append({ QDir::toNativeSeparators("tests/resources/tmp/out.jpg"), Image::Size::Full, Image::SaveResult::Blacklisted });

	assertDownload(img, &downloader, expected, false);

	m_profile->removeBlacklistedTag("tag1");
}


void ImageDownloaderTest::assertDownload(QSharedPointer<Image> img, ImageDownloader *downloader, const QList<ImageSaveResult> &expected, bool shouldExist, bool onlyCheckValues, bool sampleFallback)
{
	const bool oldSampleFallback = m_profile->getSettings()->value("Save/samplefallback", true).toBool();
	m_profile->getSettings()->setValue("Save/samplefallback", sampleFallback);

	qRegisterMetaType<QList<ImageSaveResult>>();
	QSignalSpy spy(downloader, SIGNAL(saved(QSharedPointer<Image>, QList<ImageSaveResult>)));
	QTimer::singleShot(1, downloader, SLOT(save()));
	QVERIFY(spy.wait());

	QList<QVariant> arguments = spy.takeFirst();
	auto out = arguments[0].value<QSharedPointer<Image>>();
	auto result = arguments[1].value<QList<ImageSaveResult>>();

	m_profile->getSettings()->setValue("Save/samplefallback", oldSampleFallback);

	QCOMPARE(out, img);
	QCOMPARE(result.count(), expected.count());
	for (int i = 0; i < result.count(); ++i) {
		if (!onlyCheckValues) {
			QCOMPARE(result[i].path, expected[i].path);
		}
		QCOMPARE(result[i].size, expected[i].size);
		QCOMPARE(result[i].result, expected[i].result);
	}

	for (const ImageSaveResult &res : result) {
		QFile f(res.path);
		bool exists = f.exists();
		QVERIFY(exists == shouldExist);
		if (exists) {
			f.remove();
		}
	}
}


QTEST_MAIN(ImageDownloaderTest)
