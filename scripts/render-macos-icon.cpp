// Render the editable SVG into every macOS icon size using the build's QtSvg.
#include <QGuiApplication>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

int main(int argc, char **argv)
{
  QGuiApplication app(argc, argv);
  if (argc != 3) return 1;
  QSvgRenderer svg(QString::fromLocal8Bit(argv[1]));
  if (!svg.isValid()) return 2;
  QDir output(QString::fromLocal8Bit(argv[2]));
  if (!output.mkpath(".")) return 3;
  for (int size : {16, 32, 128, 256, 512}) {
    for (int scale : {1, 2}) {
      QImage image(size * scale, size * scale, QImage::Format_ARGB32_Premultiplied);
      image.fill(Qt::transparent);
      QPainter painter(&image);
      painter.setRenderHint(QPainter::Antialiasing);
      svg.render(&painter);
      painter.end();
      const QString name = QString("icon_%1x%1%2.png").arg(size).arg(scale == 2 ? "@2x" : "");
      if (!image.save(output.filePath(name))) return 4;
    }
  }
}
