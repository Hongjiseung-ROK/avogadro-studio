/******************************************************************************
  This source file is part of the Avogadro project.
  This source code is released under the 3-Clause BSD License, (see "LICENSE").
******************************************************************************/

#include <avogadro/core/vector.h>
#include <avogadro/qtopengl/glwidget.h>
#include <avogadro/rendering/geometrynode.h>
#include <avogadro/rendering/spheregeometry.h>

#include <gtest/gtest.h>

#include <QtCore/QTimer>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QSurfaceFormat>
#include <QtWidgets/QApplication>

#include <algorithm>
#include <vector>

namespace {

class FirstFrameGLWidget : public Avogadro::QtOpenGL::GLWidget
{
public:
  using Avogadro::QtOpenGL::GLWidget::GLWidget;

  bool frameCaptured() const { return m_frameCaptured; }
  unsigned int glError() const { return m_glError; }
  unsigned char maximumRed() const { return m_maximumRed; }
  size_t redDominantPixelCount() const { return m_redDominantPixelCount; }

protected:
  void paintGL() override
  {
    Avogadro::QtOpenGL::GLWidget::paintGL();
    if (m_frameCaptured)
      return;

    const int pixelWidth = static_cast<int>(width() * devicePixelRatioF());
    const int pixelHeight = static_cast<int>(height() * devicePixelRatioF());
    std::vector<unsigned char> pixels(
      static_cast<size_t>(pixelWidth * pixelHeight * 4));

    QOpenGLFunctions* functions = context()->functions();
    functions->glReadPixels(0, 0, pixelWidth, pixelHeight, GL_RGBA,
                            GL_UNSIGNED_BYTE, pixels.data());
    m_glError = functions->glGetError();

    for (size_t i = 0; i < pixels.size(); i += 4) {
      const unsigned char red = pixels[i];
      const unsigned char green = pixels[i + 1];
      const unsigned char blue = pixels[i + 2];
      m_maximumRed = std::max(m_maximumRed, red);
      if (red > 32 && red > green + 16 && red > blue + 16)
        ++m_redDominantPixelCount;
    }

    m_frameCaptured = true;
    QTimer::singleShot(0, qApp, &QCoreApplication::quit);
  }

private:
  bool m_frameCaptured = false;
  unsigned int m_glError = GL_NO_ERROR;
  unsigned char m_maximumRed = 0;
  size_t m_redDominantPixelCount = 0;
};

TEST(GLRendererFirstFrameTest, FogDoesNotHideSolidGeometry)
{
  using Avogadro::Rendering::GeometryNode;
  using Avogadro::Rendering::SphereGeometry;

  FirstFrameGLWidget widget;
  auto* geometry = new GeometryNode;
  auto* spheres = new SphereGeometry;
  spheres->addSphere(Avogadro::Vector3f::Zero(), Avogadro::Vector3ub(255, 0, 0),
                     1.5f);
  geometry->addDrawable(spheres);
  widget.renderer().scene().rootNode().addChild(geometry);
  widget.renderer().solidPipeline().setFogEnabled(true);
  widget.renderer().resetCamera();
  widget.resize(160, 160);

  bool timedOut = false;
  QTimer timeout;
  timeout.setSingleShot(true);
  QObject::connect(&timeout, &QTimer::timeout, [&timedOut]() {
    timedOut = true;
    qApp->quit();
  });
  timeout.start(5000);

  widget.show();
  qApp->exec();
  timeout.stop();

  ASSERT_FALSE(timedOut);
  ASSERT_TRUE(widget.renderer().isValid()) << widget.renderer().error();
  ASSERT_TRUE(widget.frameCaptured());
  ASSERT_EQ(widget.glError(), static_cast<unsigned int>(GL_NO_ERROR));
  EXPECT_GT(widget.maximumRed(), 32);
  EXPECT_GT(widget.redDominantPixelCount(), 0u);
}

} // namespace

int main(int argc, char* argv[])
{
  QSurfaceFormat format;
  format.setRenderableType(QSurfaceFormat::OpenGL);
  format.setVersion(4, 1);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  QSurfaceFormat::setDefaultFormat(format);

  QApplication app(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
