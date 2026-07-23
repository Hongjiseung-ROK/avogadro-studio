/******************************************************************************
  This source file is part of the Avogadro project.
  This source code is released under the 3-Clause BSD License, (see "LICENSE").
******************************************************************************/

#include <avogadro/qtgui/molecule.h>
#include <avogadro/qtopengl/glwidget.h>

#include <QtWidgets/QApplication>

#include <iostream>

namespace {

class ObservableMolecule : public Avogadro::QtGui::Molecule
{
public:
  int changedReceiverCount() const
  {
    return receivers(SIGNAL(changed(unsigned int)));
  }

  void notifyChanged() { emit changed(Atoms); }
};

bool expectReceiverCount(const ObservableMolecule& molecule, int expected,
                         const char* context)
{
  const int observed = molecule.changedReceiverCount();
  if (observed == expected)
    return true;

  std::cerr << context << ": expected " << expected
            << " changed receivers, got " << observed << '\n';
  return false;
}

} // namespace

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  Avogadro::QtOpenGL::GLWidget widget;
  ObservableMolecule first;
  ObservableMolecule second;
  QObject observerContext;
  int externalNotifications = 0;

  QObject::connect(
    &first, &Avogadro::QtGui::Molecule::changed, &observerContext,
    [&externalNotifications](unsigned int) { ++externalNotifications; });

  if (!expectReceiverCount(first, 1, "external observer setup"))
    return 1;

  widget.setMolecule(&first);
  if (!expectReceiverCount(first, 2, "initial molecule assignment"))
    return 1;

  widget.setMolecule(&first);
  if (!expectReceiverCount(first, 2, "repeated molecule assignment"))
    return 1;

  widget.setMolecule(&second);
  if (!expectReceiverCount(first, 1, "molecule swap preserves observers") ||
      !expectReceiverCount(second, 1, "molecule swap connects widget"))
    return 1;

  first.notifyChanged();
  if (externalNotifications != 1) {
    std::cerr << "external observer did not receive the molecule change\n";
    return 1;
  }

  widget.setMolecule(nullptr);
  if (!expectReceiverCount(second, 0, "null molecule transition"))
    return 1;

  widget.setMolecule(&first);
  if (!expectReceiverCount(first, 2, "reconnect after null transition"))
    return 1;

  return 0;
}
