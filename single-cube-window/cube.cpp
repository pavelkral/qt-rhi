#include "cube.h"

void Cube::init(QRhi *rhi, QRhiResourceUpdateBatch *u)
{
    // Vertex buffer
    m_vbuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices)));
    m_vbuf->create();
    u->uploadStaticBuffer(m_vbuf.get(), vertices);

    // Index buffer
    m_ibuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indices)));
    m_ibuf->create();
    u->uploadStaticBuffer(m_ibuf.get(), indices);

    m_indexCount = sizeof(indices) / sizeof(indices[0]);
}

void Cube::draw(QRhiCommandBuffer *cb)
{
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding, m_ibuf.get(), 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(m_indexCount);
}
