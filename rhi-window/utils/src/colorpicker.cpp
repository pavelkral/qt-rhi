#include "ColorPicker.h"
#include <QColorDialog>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

ColorPicker::ColorPicker(QWidget *parent)
    : QDialog(parent), m_redSlider(nullptr), m_greenSlider(nullptr),
    m_blueSlider(nullptr), m_colorDisplayLabel(nullptr),
    m_redValueLabel(nullptr), m_greenValueLabel(nullptr),
    m_blueValueLabel(nullptr), m_pickColorButton(nullptr),
    m_htmlColorEdit(nullptr),
    m_rgbColorEdit(nullptr),
    m_hslColorEdit(nullptr){


    m_redSlider = new QSlider(Qt::Horizontal, this);
    m_redSlider->setRange(0, 255);
    m_redValueLabel = new QLabel("R: 0", this);

    m_greenSlider = new QSlider(Qt::Horizontal, this);
    m_greenSlider->setRange(0, 255);
    m_greenValueLabel = new QLabel("G: 0", this);

    m_blueSlider = new QSlider(Qt::Horizontal, this);
    m_blueSlider->setRange(0, 255);
    m_blueValueLabel = new QLabel("B: 0", this);

    m_colorDisplayLabel = new QLabel(this);
    m_colorDisplayLabel->setFixedSize(100, 50);
    m_colorDisplayLabel->setStyleSheet(
        "background-color: black; border: 1px solid gray;");
    m_colorDisplayLabel->setAlignment(Qt::AlignCenter);

    m_pickColorButton =
        new QPushButton("Pick Color from Dialog)", this);

    m_htmlColorEdit = new QLineEdit(this);
    m_htmlColorEdit->setReadOnly(true);
    m_rgbColorEdit = new QLineEdit(this);
    m_rgbColorEdit->setReadOnly(true);
    m_hslColorEdit = new QLineEdit(this);
    m_hslColorEdit->setReadOnly(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *redLayout = new QHBoxLayout();
    redLayout->addWidget(new QLabel("Red:", this));
    redLayout->addWidget(m_redSlider);
    redLayout->addWidget(m_redValueLabel);

    QHBoxLayout *greenLayout = new QHBoxLayout();
    greenLayout->addWidget(new QLabel("Green:", this));
    greenLayout->addWidget(m_greenSlider);
    greenLayout->addWidget(m_greenValueLabel);

    QHBoxLayout *blueLayout = new QHBoxLayout();
    blueLayout->addWidget(new QLabel("Blue:", this));
    blueLayout->addWidget(m_blueSlider);
    blueLayout->addWidget(m_blueValueLabel);

    mainLayout->addLayout(redLayout);
    mainLayout->addLayout(greenLayout);
    mainLayout->addLayout(blueLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_colorDisplayLabel, 0, Qt::AlignCenter);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_pickColorButton, 0, Qt::AlignCenter);

    QHBoxLayout *htmlLayout = new QHBoxLayout();
    htmlLayout->addWidget(new QLabel("HTML:", this));
    htmlLayout->addWidget(m_htmlColorEdit);
    mainLayout->addLayout(htmlLayout);

    QHBoxLayout *rgbLayout = new QHBoxLayout();
    rgbLayout->addWidget(new QLabel("RGB:", this));
    rgbLayout->addWidget(m_rgbColorEdit);
    mainLayout->addLayout(rgbLayout);

    QHBoxLayout *hslLayout = new QHBoxLayout();
    hslLayout->addWidget(new QLabel("HSL:", this));
    hslLayout->addWidget(m_hslColorEdit);
    mainLayout->addLayout(hslLayout);

    connect(m_redSlider, &QSlider::valueChanged, this,
            &ColorPicker::updateColorFromSlidersR);
    connect(m_greenSlider, &QSlider::valueChanged, this,
            &ColorPicker::updateColorFromSlidersG);
    connect(m_blueSlider, &QSlider::valueChanged, this,
            &ColorPicker::updateColorFromSlidersB);
    connect(m_pickColorButton, &QPushButton::clicked, this,
            &ColorPicker::pickColorFromScreen);

    setColor(Qt::black);
}

ColorPicker::~ColorPicker() {

}

QColor ColorPicker::color() const { return m_currentColor; }

void ColorPicker::setColor(const QColor &color) {

    if (m_currentColor != color) {
        m_currentColor = color;

        m_redSlider->setValue(m_currentColor.red());
        m_greenSlider->setValue(m_currentColor.green());
        m_blueSlider->setValue(m_currentColor.blue());

    }
}

void ColorPicker::updateColorFromSlidersR() {
    m_currentColor.setRgb(m_redSlider->value() , m_currentColor.green(),
                          m_currentColor.blue());

    qDebug() << " Current Slider values R"
             << " R:" << m_redSlider->value() << " G:" << m_greenSlider->value()
             << " B:" << m_blueSlider->value();

    qDebug()
        << "updateColorFromSliders Red : m_currentColor after setting from sliders:"
        << m_currentColor;

    m_redValueLabel->setText(QString("R: %1").arg(m_currentColor.red()));
    m_greenValueLabel->setText(QString("G: %1").arg(m_currentColor.green()));
    m_blueValueLabel->setText(QString("B: %1").arg(m_currentColor.blue()));

    updateColorDisplay();
}

void ColorPicker::updateColorFromSlidersB() {
    m_currentColor.setRgb(m_currentColor.red(), m_currentColor.green(),
                          m_blueSlider->value());

    qDebug() << "Current Slider values B"
             << " R:" << m_redSlider->value() << " G:" << m_greenSlider->value()
             << " B:" << m_blueSlider->value();

    qDebug()
        << "updateColorFromSliders Blue: m_currentColor after setting from sliders:"
        << m_currentColor; // DEBUG 4

    m_redValueLabel->setText(QString("R: %1").arg(m_currentColor.red()));
    m_greenValueLabel->setText(QString("G: %1").arg(m_currentColor.green()));
    m_blueValueLabel->setText(QString("B: %1").arg(m_currentColor.blue()));

    updateColorDisplay();
}

void ColorPicker::updateColorFromSlidersG() {

    m_currentColor.setRgb(m_currentColor.red(), m_greenSlider->value(),
                          m_currentColor.blue());

    qDebug() << "Current Slider values G:"
             << m_redSlider->value() << " G:" << m_greenSlider->value()
             << " B:" << m_blueSlider->value();

    qDebug()
        << "updateColorFromSliders Green: m_currentColor after setting from sliders:"
        << m_currentColor; // DEBUG 4

    m_redValueLabel->setText(QString("R: %1").arg(m_currentColor.red()));
    m_greenValueLabel->setText(QString("G: %1").arg(m_currentColor.green()));
    m_blueValueLabel->setText(QString("B: %1").arg(m_currentColor.blue()));

    updateColorDisplay();
}
void ColorPicker::updateColorDisplay() {

    m_colorDisplayLabel->setStyleSheet(
        QString("background-color: rgb(%1, %2, %3); border: 1px solid gray;")
            .arg(m_currentColor.red())
            .arg(m_currentColor.green())
            .arg(m_currentColor.blue()));

    // HTML (hex)  ret #RRGGBB
    m_htmlColorEdit->setText(m_currentColor.name(QColor::HexRgb).toUpper());

    // RGB "R:X G:Y B:Z"
    m_rgbColorEdit->setText(QString("%1,%2,%3")
                                .arg(m_currentColor.red())
                                .arg(m_currentColor.green())
                                .arg(m_currentColor.blue()));
    m_hslColorEdit->setText(QString("H:%1 S:%2 L:%3")
                                .arg(m_currentColor.hue())
                                .arg(m_currentColor.saturation())
                                .arg(m_currentColor.lightness()));
}

void ColorPicker::pickColorFromScreen() {
    QColorDialog dialog(m_currentColor, this);
    dialog.setOption(QColorDialog::ShowAlphaChannel, false);
    dialog.setOption(QColorDialog::DontUseNativeDialog,
                     true);

    if (dialog.exec() == QDialog::Accepted) {

        QColor selectedFromDialog = dialog.selectedColor();
        qDebug() << "Dialog selected color:" << selectedFromDialog;
        /// setColor(selectedFromDialog);
        m_currentColor = selectedFromDialog;
        qDebug() << "red:" << m_currentColor.red();
        qDebug() << "blue:" << m_currentColor.blue();
        qDebug() << "green:" << m_currentColor.green();

        m_redSlider->setValue(m_currentColor.red());
        m_greenSlider->setValue(m_currentColor.green());
        m_blueSlider->setValue(m_currentColor.blue());

    }
}
