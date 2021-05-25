#include "contentwidget.h"
#include "constant.h"

#include <QVBoxLayout>
#include <QQuickItem>

ContentWidget::ContentWidget(QWidget *parent) : BaseWidget(parent)
{
    setBackgroundColor(10, 10, 10);

    initLayout();
    initConnection();
}

void ContentWidget::initLayout()
{
    QVBoxLayout *layout = new QVBoxLayout;

    m_surfaceWid = new QuickInterfaceWidget(this);
    m_surfaceWid->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_surfaceWid->setSource(QUrl("qrc:/video.qml"));
    // set video surface transparent.
    m_surfaceWid->setClearColor(QColor(Qt::transparent));
    QObject* qmlMediaPlayer = m_surfaceWid->rootObject()->findChild<QObject*>("mediaPlayer");
    m_player = qvariant_cast<QMediaPlayer *>(qmlMediaPlayer->property("mediaObject"));

    layout->addWidget(m_surfaceWid);
    layout->setMargin(0);
    layout->setSpacing(0);

    setLayout(layout);
}

void ContentWidget::initConnection()
{
    connect(m_surfaceWid, SIGNAL(contentOneClick()), this, SIGNAL(surfaceOneClick()));
    connect(m_surfaceWid, SIGNAL(contentDoubleClick()), this, SIGNAL(surfaceDoubleClick()));
}

ContentWidget::~ContentWidget()
{
}
