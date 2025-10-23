#include "rhi-widget.h"

#include "model.h"

#include <QFile>
#include <QMimeData>
#include <QMouseEvent>

RhiWidget::RhiWidget()
{
    setAcceptDrops(true);
    setSampleCount(4);
}

void RhiWidget::initialize(QRhiCommandBuffer *)
{
    if (rhi_ != rhi()) {
        rhi_ = rhi();
    }
    QString url = QCoreApplication::applicationDirPath() + "/assets/models/jet/jet.fbx";

    if (!QFile::exists(url)) {
        qWarning() << "Soubor neexistuje:" << url;
        return;
    }
    model = std::make_unique<Model>(url);
    model->create(rhi_, renderTarget());
}

void RhiWidget::render(QRhiCommandBuffer *cb)
{
    const auto rub  = rhi_->nextResourceUpdateBatch();
    const auto rtsz = renderTarget()->pixelSize();

    mvp_.setToIdentity();
    mvp_.perspective(45.0f, rtsz.width() / (float)rtsz.height(), 0.01f, 3000.0f);
    mvp_.translate({ 0.0f, 0.0f, -8.0f });
    mvp_.rotate(rotation_);
    mvp_.scale(scale_);

    model->upload(rub, mvp_);


    // for (auto& item : items_) {
    //     item->create(rhi_, renderTarget());
    //     item->upload(rub, mvp_);
    // }

    cb->beginPass(renderTarget(), Qt::gray, { 1.0f, 0 }, rub);

    // for (auto& item : items_) {
    //     item->draw(cb, { 0, 0, static_cast<float>(rtsz.width()), static_cast<float>(rtsz.height()) });
    // }
    model->draw(cb, { 0, 0, static_cast<float>(rtsz.width()), static_cast<float>(rtsz.height()) });
    cb->endPass();
}

void RhiWidget::mousePressEvent(QMouseEvent *event)
{
    last_pos_ = QVector2D(event->position());
    QWidget::mousePressEvent(event);
}

void RhiWidget::mouseMoveEvent(QMouseEvent *event)
{
    const auto diff = QVector2D(event->position()) - last_pos_;
    last_pos_       = QVector2D(event->position());

    const auto axis = QVector3D(diff.y() / 4, diff.x() / 4, 0.0).normalized();
    rotation_       = QQuaternion::fromAxisAndAngle(axis, diff.length()) * rotation_;

    update();

    QWidget::mouseMoveEvent(event);
}

void RhiWidget::wheelEvent(QWheelEvent *event)
{
    scale_ *= event->angleDelta().y() < 0 ? 0.95f : 1.05f;

    update();

    QWidget::wheelEvent(event);
}

void RhiWidget::dragEnterEvent(QDragEnterEvent *event)
{
    const auto mimedata = event->mimeData();
    if (mimedata->hasUrls()) {
        event->acceptProposedAction();
    }
    else {
        event->ignore();
    }
}

void RhiWidget::dropEvent(QDropEvent *event)
{
   // items_.clear();

    // const auto mimedata = event->mimeData();
    // for (auto& url : mimedata->urls()) {
    //     items_.emplace_back(std::make_unique<Model>(url.toLocalFile()));
    // }

    update();
}
