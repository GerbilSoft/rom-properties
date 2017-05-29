/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * MessageWidget.hpp: Message widget.                                      *
 *                                                                         *
 * Copyright (c) 2014-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "MessageWidget.hpp"

// Qt includes.
#include <QtCore/QTimer>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>

// Qt widgets.
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QToolButton>
#include <QStyle>

// Qt animation includes.
#include <QtCore/QTimeLine>

/** MessageWidgetPrivate **/

class MessageWidgetPrivate
{
	public:
		explicit MessageWidgetPrivate(MessageWidget *q);
		~MessageWidgetPrivate();

	protected:
		MessageWidget *const q_ptr;
		Q_DECLARE_PUBLIC(MessageWidget)
	private:
		Q_DISABLE_COPY(MessageWidgetPrivate)

	public:
		struct Ui_MessageWidget {
			QHBoxLayout *hboxMain;
			QFrame *content;
			QHBoxLayout *hboxFrame;
			QLabel *lblIcon;
			QLabel *lblMessage;
			QToolButton *btnDismiss;

			void setupUi(QWidget *MessageWidget);
		};
		Ui_MessageWidget ui;

		// Icon being displayed.
		MessageWidget::MsgIcon icon;
		static const int iconSz = 22;
		void setIcon(MessageWidget::MsgIcon icon);

		// Message timeout.
		QTimer *tmrTimeout;
		bool timeout;	// True if message was dismissed via timeout.

		// Colors.
		// TODO: Use system colors on KDE?
		static const QRgb colorCritical = 0xEE4444;
		static const QRgb colorQuestion = 0x66EE66;
		static const QRgb colorWarning = 0xEECC66;
		static const QRgb colorInformation = 0x66CCEE;

		// Animation.
		QTimeLine *timeLine;
		int calcBestHeight(void) const;
		bool animateOnShow;
};

MessageWidgetPrivate::MessageWidgetPrivate(MessageWidget *q)
	: q_ptr(q)
	, icon(MessageWidget::ICON_NONE)
	, tmrTimeout(new QTimer(q))
	, timeout(false)
	, timeLine(new QTimeLine(500, q))
	, animateOnShow(false)
{ }

MessageWidgetPrivate::~MessageWidgetPrivate()
{ }

/**
 * Initialize the UI.
 * @param MessageWidget MessageWidget.
 */
void MessageWidgetPrivate::Ui_MessageWidget::setupUi(QWidget *MessageWidget)
{
	if (MessageWidget->objectName().isEmpty())
		MessageWidget->setObjectName(QLatin1String("MessageWidget"));

	hboxMain = new QHBoxLayout(MessageWidget);
	hboxMain->setContentsMargins(2, 2, 2, 2);
	hboxMain->setObjectName(QLatin1String("hboxMain"));

	content = new QFrame(MessageWidget);
	content->setObjectName(QLatin1String("content"));
	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(content->sizePolicy().hasHeightForWidth());
	content->setSizePolicy(sizePolicy);
	content->setFrameShape(QFrame::NoFrame);
	content->setLineWidth(0);

	hboxFrame = new QHBoxLayout(content);
	hboxFrame->setContentsMargins(0, 0, 0, 0);
	hboxFrame->setObjectName(QLatin1String("hboxFrame"));

	lblIcon = new QLabel(content);
	lblIcon->setObjectName(QLatin1String("lblIcon"));
	QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(lblIcon->sizePolicy().hasHeightForWidth());
	lblIcon->setSizePolicy(sizePolicy1);
	lblIcon->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

	hboxFrame->addWidget(lblIcon);
	hboxFrame->setAlignment(lblIcon, Qt::AlignTop);

	lblMessage = new QLabel(content);
	lblMessage->setObjectName(QLatin1String("lblMessage"));
	lblMessage->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
	lblMessage->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

	hboxFrame->addWidget(lblMessage);
	hboxFrame->setAlignment(lblMessage, Qt::AlignTop);

	btnDismiss = new QToolButton(content);
	btnDismiss->setObjectName(QLatin1String("btnDismiss"));
	btnDismiss->setFocusPolicy(Qt::NoFocus);
	QIcon icon = btnDismiss->style()->standardIcon(QStyle::SP_DialogCloseButton);
	btnDismiss->setIcon(icon);
	const int szBtn = iconSz + ((iconSz/4) & ~1);
	btnDismiss->setMaximumSize(szBtn, szBtn);

	hboxFrame->addWidget(btnDismiss);
	hboxFrame->setAlignment(btnDismiss, Qt::AlignTop);

	hboxMain->addWidget(content);

	QMetaObject::connectSlotsByName(MessageWidget);
}

/**
 * Set the icon.
 * @param icon Icon to set.
 */
void MessageWidgetPrivate::setIcon(MessageWidget::MsgIcon icon)
{
	if (icon < MessageWidget::ICON_NONE || icon >= MessageWidget::ICON_MAX)
		icon = MessageWidget::ICON_NONE;
	if (this->icon == icon)
		return;
	this->icon = icon;

	QStyle::StandardPixmap standardPixmap;
	bool hasStandardPixmap = false;
	switch (icon) {
		case MessageWidget::ICON_NONE:
		default:
			hasStandardPixmap = false;
			break;
		case MessageWidget::ICON_CRITICAL:
			standardPixmap = QStyle::SP_MessageBoxCritical;
			hasStandardPixmap = true;
			break;
		case MessageWidget::ICON_QUESTION:
			// FIXME: May not be available on some systems...
			standardPixmap = QStyle::SP_MessageBoxQuestion;
			hasStandardPixmap = true;
			break;
		case MessageWidget::ICON_WARNING:
			standardPixmap = QStyle::SP_MessageBoxWarning;
			hasStandardPixmap = true;
			break;
		case MessageWidget::ICON_INFORMATION:
			standardPixmap = QStyle::SP_MessageBoxInformation;
			hasStandardPixmap = true;
			break;
	}

	if (!hasStandardPixmap) {
		ui.lblIcon->setVisible(false);
	} else {
		QIcon icon = ui.lblIcon->style()->standardIcon(standardPixmap);
		ui.lblIcon->setPixmap(icon.pixmap(iconSz, iconSz));
		ui.lblIcon->setVisible(true);
	}

	Q_Q(MessageWidget);
	if (q->isVisible())
		q->update();
}

/**
 * Calculate the best height for the widget.
 * @return Best height.
 */
int MessageWidgetPrivate::calcBestHeight(void) const
{
	int height = ui.content->sizeHint().height();
	height += ui.hboxMain->contentsMargins().top();
	height += ui.hboxMain->contentsMargins().bottom();
	height += ui.hboxFrame->contentsMargins().top();
	height += ui.hboxFrame->contentsMargins().bottom();
	return height;
}

/** MessageWidget **/

MessageWidget::MessageWidget(QWidget *parent)
	: super(parent)
	, d_ptr(new MessageWidgetPrivate(this))
{
	Q_D(MessageWidget);
	d->ui.setupUi(this);
	d->setIcon(d->icon);

	// Connect the timer signal.
	QObject::connect(d->tmrTimeout, SIGNAL(timeout()),
			 this, SLOT(tmrTimeout_timeout()));

	// Connect the timeline signals.
	QObject::connect(d->timeLine, SIGNAL(valueChanged(qreal)),
			 this, SLOT(timeLineChanged_slot(qreal)));
	QObject::connect(d->timeLine, SIGNAL(finished()),
			 this, SLOT(timeLineFinished_slot()));
}

MessageWidget::~MessageWidget()
{
	Q_D(MessageWidget);
	delete d;
}

/** Events. **/

/**
 * Paint event.
 * @param event QPaintEvent.
 */
void MessageWidget::paintEvent(QPaintEvent *event)
{
	// Call the superclass paintEvent first.
	super::paintEvent(event);

	QPainter painter(this);

	// Drawing rectangle should be this->rect(),
	// minus one pixel width and height.
	QRect drawRect(this->rect());
	drawRect.setWidth(drawRect.width() - 1);
	drawRect.setHeight(drawRect.height() - 1);

	// Determine the background color based on icon.
	QBrush bgColor;
	Q_D(MessageWidget);
	switch (d->icon) {
		case ICON_CRITICAL:
			bgColor = QColor(MessageWidgetPrivate::colorCritical);
			break;
		case ICON_QUESTION:
			bgColor = QColor(MessageWidgetPrivate::colorQuestion);
			break;
		case ICON_WARNING:
			bgColor = QColor(MessageWidgetPrivate::colorWarning);
			break;
		case ICON_INFORMATION:
			bgColor = QColor(MessageWidgetPrivate::colorInformation);
			break;
		default:
			// FIXME: This looks terrible.
			bgColor = QColor(Qt::white);
			break;
	}

	if (bgColor != QColor(Qt::white)) {
		painter.setPen(QColor(Qt::black));
		painter.setBrush(bgColor);
		painter.drawRoundedRect(drawRect, 5.0, 5.0);
	}
}

/**
 * Show event.
 * @param event QShowEent.
 */
void MessageWidget::showEvent(QShowEvent *event)
{
	// Call the superclass showEvent.
	super::showEvent(event);

	Q_D(MessageWidget);
	d->timeout = false;
	if (d->animateOnShow) {
		// Start the animation.
		d->animateOnShow = false;
		d->timeLine->setDirection(QTimeLine::Forward);
		if (d->timeLine->state() == QTimeLine::NotRunning) {
			d->timeLine->start();
		}
	}
}

/** Slots. **/

/**
 * Show a message.
 * @param msg Message text. (supports Qt RichText formatting)
 * @param icon Icon.
 * @param timeout Timeout, in milliseconds. (0 for no timeout)
 * @param closeOnDestroy Close the message when the specified QObject is destroyed.
 */
void MessageWidget::showMessage(const QString &msg, MsgIcon icon, int timeout, QObject *closeOnDestroy)
{
	Q_D(MessageWidget);
	d->ui.lblMessage->setText(msg);
	d->setIcon(icon);
	if (icon == ICON_CRITICAL) {
		// Use white text.
		d->ui.lblMessage->setStyleSheet(QLatin1String("QLabel { color: white; }"));
	} else {
		// Use black text.
		d->ui.lblMessage->setStyleSheet(QLatin1String("QLabel { color: black; }"));
	}

	// Set up the timer.
	d->tmrTimeout->stop();
	d->tmrTimeout->setInterval(timeout);

	// Close the widget when the specified QObject is destroyed.
	if (closeOnDestroy) {
		connect(closeOnDestroy, SIGNAL(destroyed()),
			this, SLOT(on_btnDismiss_clicked()),
			Qt::UniqueConnection);
	}

	// If the widget is already visible, just update it.
	if (this->isVisible()) {
		update();
		return;
	}

	// Do an animated show.
	this->showAnimated();
}

/**
 * Show the MessageWidget using animation.
 */
void MessageWidget::showAnimated(void)
{
	Q_D(MessageWidget);
	setFixedHeight(0);
	d->ui.content->setGeometry(0, 0, width(), d->calcBestHeight());
	d->animateOnShow = true;
	d->tmrTimeout->stop();
	this->show();
}

/**
 * Hide the MessageWidget using animation.
 */
void MessageWidget::hideAnimated(void)
{
	Q_D(MessageWidget);

	// Start the animation.
	d->animateOnShow = false;
	d->tmrTimeout->stop();
	d->timeLine->setDirection(QTimeLine::Backward);
	if (d->timeLine->state() == QTimeLine::NotRunning) {
		d->timeLine->start();
	}
}

/**
 * Message timer has expired.
 */
void MessageWidget::tmrTimeout_timeout(void)
{
	// Hide the message using animation.
	Q_D(MessageWidget);
	d->timeout = true;
	this->hideAnimated();
}

/**
 * Animation timeline has changed.
 * @param value Timeline value.
 */
void MessageWidget::timeLineChanged_slot(qreal value)
{
	Q_D(MessageWidget);
	this->setFixedHeight(qMin(value, qreal(1.0)) * d->calcBestHeight());
}

/**
 * Animation timeline has finished.
 */
void MessageWidget::timeLineFinished_slot(void)
{
	Q_D(MessageWidget);
	if (d->timeLine->direction() == QTimeLine::Forward) {
		// Make sure the widget is full-size.
		this->setFixedHeight(d->calcBestHeight());

		// Start the timeout timer, if specified.
		d->timeout = false;
		if (d->tmrTimeout->interval() > 0)
			d->tmrTimeout->start();
	} else {
		// Message is dismissed.
		// NOTE: This used to call this->hide(),
		// but that causes a deadlock when
		// used with MessageWidgetStack.
		d->tmrTimeout->stop();
		emit dismissed(d->timeout);
	}
}

/**
 * "Dismiss" button has been clicked.
 */
void MessageWidget::on_btnDismiss_clicked(void)
{
	// Hide the message using animation.
	Q_D(MessageWidget);
	if (d->timeLine->state() == QTimeLine::NotRunning) {
		d->tmrTimeout->stop();
		d->timeout = false;
		this->hideAnimated();
	}
}
