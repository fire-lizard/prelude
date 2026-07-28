#pragma once
// container for new settings

class CustomOptions {
public:
	int cameraBorderPanningEnabled;
	CustomOptions() {
		cameraBorderPanningEnabled = 1;
	}

	void toggleCameraBorderPanning() {
		cameraBorderPanningEnabled = !cameraBorderPanningEnabled;
	}
};