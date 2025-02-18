#include "search-window.h"
#include <QCalendarWidget>
#include <QCompleter>
#include <QCryptographicHash>
#include <QFile>
#include <QFileDialog>
#include <QRegularExpression>
#include <QSettings>
#include <ui_search-window.h>
#include "functions.h"
#include "models/profile.h"
#include "ui/text-edit.h"


SearchWindow::SearchWindow(QString tags, Profile *profile, QWidget *parent)
	: QDialog(parent), ui(new Ui::SearchWindow), m_profile(profile)
{
	setAttribute(Qt::WA_DeleteOnClose);
	ui->setupUi(this);

	m_calendar = new QCalendarWidget(this);
		m_calendar->setWindowFlags(Qt::Window);
		m_calendar->setLocale(QLocale(m_profile->getSettings()->value("language", "English").toString().toLower().left(2)));
		m_calendar->setWindowIcon(QIcon(":/images/icon.ico"));
		m_calendar->setWindowTitle(tr("Choose a date"));
		m_calendar->setDateRange(QDate(2000, 1, 1), QDateTime::currentDateTime().date().addDays(1));
		m_calendar->setSelectedDate(QDateTime::currentDateTime().date());
		connect(m_calendar, &QCalendarWidget::activated, this, &SearchWindow::setDate);
		connect(m_calendar, &QCalendarWidget::activated, m_calendar, &QCalendarWidget::close);
	connect(ui->buttonCalendar, &QPushButton::clicked, m_calendar, &QCalendarWidget::show);

	m_tags = new TextEdit(profile, this);
		m_tags->setContextMenuPolicy(Qt::CustomContextMenu);
		auto *completer = new QCompleter(profile->getAutoComplete(), m_tags);
			completer->setCaseSensitivity(Qt::CaseInsensitive);
			completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
			m_tags->setCompleter(completer);
		connect(m_tags, &TextEdit::returnPressed, this, &SearchWindow::accept);
	ui->formLayout->setWidget(0, QFormLayout::FieldRole, m_tags);

	const QStringList orders { "id", "id_desc", "score_asc", "score", "mpixels_asc", "mpixels", "filesize", "landscape", "portrait", "favcount", "rank" };
	const QStringList ratings { "rating:general", "-rating:general", "rating:safe", "-rating:safe", "rating:questionable", "-rating:questionable", "rating:explicit", "-rating:explicit" };
	const QStringList status { "deleted", "active", "flagged", "pending", "any" };

	if (tags.contains("order:")) {
		static const QRegularExpression reg("order:([^ ]+)");
		auto match = reg.match(tags);
		ui->comboOrder->setCurrentIndex(orders.indexOf(match.captured(1)) + 1);
		tags.remove(match.captured(0));
	}
	if (tags.contains("rating:")) {
		static const QRegularExpression reg("-?rating:[^ ]+");
		auto match = reg.match(tags);
		ui->comboRating->setCurrentIndex(ratings.indexOf(match.captured(0)) + 1);
		tags.remove(match.captured(0));
	}
	if (tags.contains("status:")) {
		static const QRegularExpression reg("status:([^ ]+)");
		auto match = reg.match(tags);
		ui->comboStatus->setCurrentIndex(status.indexOf(match.captured(1)) + 1);
		tags.remove(match.captured(0));
	}
	if (tags.contains("date:")) {
		static const QRegularExpression reg("date:([^ ]+)");
		auto match = reg.match(tags);
		m_calendar->setSelectedDate(QDate::fromString(match.captured(1), QStringLiteral("MM/dd/yyyy")));
		ui->lineDate->setText(match.captured(1));
		tags.remove(match.captured(0));
	}

	m_tags->setText(tags);
}
SearchWindow::~SearchWindow()
{
	delete ui;
}

QString SearchWindow::generateSearch(const QString &additional) const
{
	const QStringList orders { "id", "id_desc", "score_asc", "score", "mpixels_asc", "mpixels", "filesize", "landscape", "portrait", "favcount", "rank" };
	const QStringList ratings { "rating:general", "-rating:general", "rating:safe", "-rating:safe", "rating:questionable", "-rating:questionable", "rating:explicit", "-rating:explicit" };
	const QStringList status = { "deleted", "active", "flagged", "pending", "any" };

	QString prefix = !additional.isEmpty() ? additional + " " : QString();
	QString search = prefix + m_tags->toPlainText();
	if (ui->comboStatus->currentIndex() != 0) {
		search += " status:" + status.at(ui->comboStatus->currentIndex() - 1);
	}
	if (ui->comboOrder->currentIndex() != 0) {
		search += " order:" + orders.at(ui->comboOrder->currentIndex() - 1);
	}
	if (ui->comboRating->currentIndex() != 0) {
		search += " " + ratings.at(ui->comboRating->currentIndex() - 1);
	}
	if (!ui->lineDate->text().isEmpty()) {
		search += " date:" + ui->lineDate->text();
	}

	return search.trimmed();
}

void SearchWindow::setDate(QDate d)
{
	ui->lineDate->setText(d.toString(QStringLiteral("MM/dd/yyyy")));
}

void SearchWindow::accept()
{
	emit accepted(generateSearch());
	QDialog::accept();
}

void SearchWindow::on_buttonImage_clicked()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Search an image"), m_profile->getSettings()->value("Save/path").toString(), QStringLiteral("Images (*.png *.gif *.jpg *.jpeg)"));
	QFile f(path);
	QString md5;
	if (f.exists()) {
		f.open(QFile::ReadOnly);
		md5 = QCryptographicHash::hash(f.readAll(), QCryptographicHash::Md5).toHex();
	}

	emit accepted(generateSearch(!md5.isEmpty() ? "md5:" + md5 : QString()));
	QDialog::accept();
}
