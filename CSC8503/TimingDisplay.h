#pragma once

#include <vector>
#include <chrono>
#include <numeric>
#include <iomanip>
#include <sstream>
#include "Vector.h"

namespace NCL::CSC8503 {

	struct TimingData {
		double frameTimeMs = 0.0;
		double physicsTimeMs = 0.0;
		double renderTimeMs = 0.0;
		double averageFrameTimeMs = 0.0;
		double averagePhysicsTimeMs = 0.0;
		double averageRenderTimeMs = 0.0;
		double averageFPS = 0.0;
	};

	class TimingDisplay {
	public:
		static constexpr size_t SAMPLE_SIZE = 60;

		using Clock = std::chrono::high_resolution_clock;
		using TimePoint = std::chrono::high_resolution_clock::time_point;

		TimingDisplay() = default;

		// Use the actual dt from the timer - this is the TRUE frame time
		void RecordFrameTime(float dtSeconds) {
			double frameTimeMs = dtSeconds * 1000.0;
			frameTimeSamples.push_back(frameTimeMs);
			if (frameTimeSamples.size() > SAMPLE_SIZE) {
				frameTimeSamples.erase(frameTimeSamples.begin());
			}
			RecalculateAverages();
		}

		// For physics timing - call this around physics.Update()
		void StartPhysicsTiming() {
			physicsStart = Clock::now();
		}

		void EndPhysicsTiming() {
			TimePoint physicsEnd = Clock::now();
			double physicsTimeMs = std::chrono::duration<double, std::milli>(physicsEnd - physicsStart).count();
			physicsTimeSamples.push_back(physicsTimeMs);
			if (physicsTimeSamples.size() > SAMPLE_SIZE) {
				physicsTimeSamples.erase(physicsTimeSamples.begin());
			}
			RecalculateAverages();
		}

		// For rendering timing - call this around rendering operations
		void StartRenderTiming() {
			renderStart = Clock::now();
		}

		void EndRenderTiming() {
			TimePoint renderEnd = Clock::now();
			double renderTimeMs = std::chrono::duration<double, std::milli>(renderEnd - renderStart).count();
			renderTimeSamples.push_back(renderTimeMs);
			if (renderTimeSamples.size() > SAMPLE_SIZE) {
				renderTimeSamples.erase(renderTimeSamples.begin());
			}
			RecalculateAverages();
		}

		const TimingData& GetTimingData() const {
			return currentData;
		}

		double GetAverageFPS() const {
			return currentData.averageFPS;
		}

		double GetAverageFrameTimeMs() const {
			return currentData.averageFrameTimeMs;
		}

		double GetAveragePhysicsTimeMs() const {
			return currentData.averagePhysicsTimeMs;
		}

		double GetAverageRenderTimeMs() const {
			return currentData.averageRenderTimeMs;
		}

		// Helper to format timing values
		static std::string FormatTime(double ms) {
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(2) << ms;
			return oss.str();
		}

		// Helper to format FPS
		static std::string FormatFPS(double fps) {
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(1) << fps;
			return oss.str();
		}

	private:
		std::vector<double> frameTimeSamples;
		std::vector<double> physicsTimeSamples;
		std::vector<double> renderTimeSamples;
		TimingData currentData;

		TimePoint physicsStart;
		TimePoint renderStart;

		void RecalculateAverages() {
			if (!frameTimeSamples.empty()) {
				double totalFrameTime = 0.0;
				for (double sample : frameTimeSamples) {
					totalFrameTime += sample;
				}
				currentData.averageFrameTimeMs = totalFrameTime / frameTimeSamples.size();

				// FPS is 1000 / average frame time in ms
				if (currentData.averageFrameTimeMs > 0.0) {
					currentData.averageFPS = 1000.0 / currentData.averageFrameTimeMs;
				}
			}

			if (!physicsTimeSamples.empty()) {
				double totalPhysicsTime = 0.0;
				for (double sample : physicsTimeSamples) {
					totalPhysicsTime += sample;
				}
				currentData.averagePhysicsTimeMs = totalPhysicsTime / physicsTimeSamples.size();
			}

			if (!renderTimeSamples.empty()) {
				double totalRenderTime = 0.0;
				for (double sample : renderTimeSamples) {
					totalRenderTime += sample;
				}
				currentData.averageRenderTimeMs = totalRenderTime / renderTimeSamples.size();
			}
		}
	};

}