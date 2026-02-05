import sys
from dataclasses import dataclass

from PySide6 import QtCore, QtGui, QtWidgets


@dataclass
class FlareInputs:
    nominal_flare_time: float
    nominal_flare_alt: float
    min_flare_alt: float
    min_ttd: float
    headwind: float
    wind_valid: bool
    terrain_confidence: float
    headwind_gain: float
    terrain_penalty: float
    adj_limit: float


@dataclass
class FlareResult:
    adjustment_factor: float
    adjusted_flare_time: float
    adjusted_flare_alt: float
    min_ttd_enforced: bool
    min_alt_enforced: bool


class SmartFlareModel:
    @staticmethod
    def compute(inputs: FlareInputs) -> FlareResult:
        wind_score = inputs.headwind_gain * min(inputs.headwind, 20.0) if inputs.wind_valid else 0.0
        terrain_score = 0.0 if inputs.terrain_confidence > 0.5 else -inputs.terrain_penalty
        raw_factor = wind_score + terrain_score
        adjustment_factor = max(-inputs.adj_limit, min(raw_factor, inputs.adj_limit))

        adjusted_flare_time = inputs.nominal_flare_time * (1.0 + adjustment_factor)
        adjusted_flare_alt = inputs.nominal_flare_alt * (1.0 + adjustment_factor)

        min_alt_enforced = adjusted_flare_alt < inputs.min_flare_alt
        if min_alt_enforced:
            adjusted_flare_alt = inputs.min_flare_alt

        min_ttd_enforced = adjusted_flare_time < inputs.min_ttd
        if min_ttd_enforced:
            adjusted_flare_time = inputs.min_ttd

        return FlareResult(
            adjustment_factor=adjustment_factor,
            adjusted_flare_time=adjusted_flare_time,
            adjusted_flare_alt=adjusted_flare_alt,
            min_ttd_enforced=min_ttd_enforced,
            min_alt_enforced=min_alt_enforced,
        )


class PlotWidget(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumHeight(320)
        self._result = None

    def set_result(self, result: FlareResult, inputs: FlareInputs) -> None:
        self._result = (result, inputs)
        self.update()

    def paintEvent(self, event: QtGui.QPaintEvent) -> None:
        super().paintEvent(event)
        if not self._result:
            return

        result, inputs = self._result
        painter = QtGui.QPainter(self)
        painter.setRenderHint(QtGui.QPainter.Antialiasing)

        rect = self.rect().adjusted(40, 20, -20, -40)
        painter.fillRect(rect, QtGui.QColor(248, 248, 248))

        painter.setPen(QtGui.QPen(QtGui.QColor(80, 80, 80), 1))
        painter.drawRect(rect)
        painter.drawText(rect.left() - 30, rect.bottom(), "0")
        painter.drawText(rect.left() - 30, rect.top() + 5, f"{inputs.nominal_flare_alt:.0f}m")
        painter.drawText(rect.left(), rect.bottom() + 20, "t=0s")
        painter.drawText(rect.right() - 30, rect.bottom() + 20, f"{inputs.nominal_flare_time:.0f}s")

        def map_point(time_s: float, alt_m: float) -> QtCore.QPointF:
            x = rect.left() + (time_s / inputs.nominal_flare_time) * rect.width()
            y = rect.bottom() - (alt_m / inputs.nominal_flare_alt) * rect.height()
            return QtCore.QPointF(x, y)

        painter.setPen(QtGui.QPen(QtGui.QColor(120, 120, 180), 2))
        painter.drawLine(map_point(0.0, inputs.nominal_flare_alt), map_point(inputs.nominal_flare_time, 0.0))

        flare_point = map_point(result.adjusted_flare_time, result.adjusted_flare_alt)
        painter.setPen(QtGui.QPen(QtGui.QColor(200, 80, 60), 3))
        painter.drawEllipse(flare_point, 5, 5)
        painter.drawText(flare_point + QtCore.QPointF(8, -8), "Flare")

        painter.setPen(QtGui.QPen(QtGui.QColor(200, 80, 60), 2, QtCore.Qt.DashLine))
        painter.drawLine(flare_point, map_point(inputs.nominal_flare_time, 0.0))


class SmartLandingFlareSim(QtWidgets.QWidget):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Smart Landing Flare Simulator")
        self._build_ui()
        self._update_simulation()

    def _build_ui(self) -> None:
        layout = QtWidgets.QVBoxLayout(self)
        self.plot = PlotWidget()
        layout.addWidget(self.plot)

        form = QtWidgets.QFormLayout()
        layout.addLayout(form)

        self.headwind = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.headwind.setRange(0, 20)
        self.headwind.setValue(8)
        form.addRow("Headwind (m/s)", self.headwind)

        self.wind_valid = QtWidgets.QCheckBox("Wind estimate valid")
        self.wind_valid.setChecked(True)
        form.addRow("Wind validity", self.wind_valid)

        self.terrain_conf = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.terrain_conf.setRange(0, 100)
        self.terrain_conf.setValue(80)
        form.addRow("Terrain confidence (%)", self.terrain_conf)

        self.headwind_gain = QtWidgets.QDoubleSpinBox()
        self.headwind_gain.setRange(0.0, 0.1)
        self.headwind_gain.setSingleStep(0.005)
        self.headwind_gain.setValue(0.015)
        form.addRow("Headwind gain", self.headwind_gain)

        self.terrain_penalty = QtWidgets.QDoubleSpinBox()
        self.terrain_penalty.setRange(0.0, 0.2)
        self.terrain_penalty.setSingleStep(0.01)
        self.terrain_penalty.setValue(0.05)
        form.addRow("Terrain penalty", self.terrain_penalty)

        self.adj_limit = QtWidgets.QDoubleSpinBox()
        self.adj_limit.setRange(0.05, 0.5)
        self.adj_limit.setSingleStep(0.05)
        self.adj_limit.setValue(0.3)
        form.addRow("Adjustment limit", self.adj_limit)

        self.status = QtWidgets.QLabel()
        layout.addWidget(self.status)

        for widget in (
            self.headwind,
            self.wind_valid,
            self.terrain_conf,
            self.headwind_gain,
            self.terrain_penalty,
            self.adj_limit,
        ):
            if isinstance(widget, QtWidgets.QCheckBox):
                widget.stateChanged.connect(self._update_simulation)
            else:
                widget.valueChanged.connect(self._update_simulation)

    def _update_simulation(self) -> None:
        inputs = FlareInputs(
            nominal_flare_time=8.0,
            nominal_flare_alt=12.0,
            min_flare_alt=6.0,
            min_ttd=2.0,
            headwind=float(self.headwind.value()),
            wind_valid=self.wind_valid.isChecked(),
            terrain_confidence=self.terrain_conf.value() / 100.0,
            headwind_gain=self.headwind_gain.value(),
            terrain_penalty=self.terrain_penalty.value(),
            adj_limit=self.adj_limit.value(),
        )
        result = SmartFlareModel.compute(inputs)
        self.plot.set_result(result, inputs)

        status_lines = [
            f"Adjustment factor: {result.adjustment_factor:+.2f}",
            f"Adjusted flare time: {result.adjusted_flare_time:.2f}s",
            f"Adjusted flare altitude: {result.adjusted_flare_alt:.2f}m",
        ]
        if result.min_alt_enforced:
            status_lines.append("Minimum flare altitude enforced.")
        if result.min_ttd_enforced:
            status_lines.append("Minimum time-to-touchdown enforced.")
        self.status.setText(" | ".join(status_lines))


def main() -> None:
    app = QtWidgets.QApplication(sys.argv)
    widget = SmartLandingFlareSim()
    widget.resize(760, 520)
    widget.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
