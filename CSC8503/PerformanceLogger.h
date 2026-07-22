#pragma once

#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include "TimingDisplay.h"

namespace NCL::CSC8503 {

	struct PerformanceRecord {
		double timestamp = 0.0;
		double averageFPS = 0.0;
		double averageFrameTimeMs = 0.0;
		double averagePhysicsTimeMs = 0.0;
		double averageRenderTimeMs = 0.0;
		int objectCount = 0;
		size_t wastedMemoryBytes = 0;  // For DOD/SoA garbage data tracking

		std::string ToCSVLine() const {
			char buffer[256];
			snprintf(buffer, sizeof(buffer),
				"%.2f,%.1f,%.2f,%.2f,%.2f,%d,%zu",
				timestamp,
				averageFPS,
				averageFrameTimeMs,
				averagePhysicsTimeMs,
				averageRenderTimeMs,
				objectCount,
				wastedMemoryBytes
			);
			return std::string(buffer);
		}
	};

	class PerformanceLogger {
	public:
		PerformanceLogger(const std::string& filename)
			: filename(filename),
			logInterval(1.0),  // Log every 1 second
			timeSinceLastLog(0.0),
			startTime(std::chrono::high_resolution_clock::now()) {
			InitializeFile();
		}

		~PerformanceLogger() {
			Flush();
			if (csvFile.is_open()) {
				csvFile.close();
			}
		}

		void SetLogInterval(double intervalSeconds) {
			logInterval = intervalSeconds;
		}

		void Update(float dt, const TimingDisplay& timingDisplay, int objectCount, size_t wastedMemoryBytes = 0) {
			timeSinceLastLog += dt;

			if (timeSinceLastLog >= logInterval) {
				RecordFrame(timingDisplay, objectCount, wastedMemoryBytes);
				timeSinceLastLog = 0.0;
			}
		}

		void RecordFrame(const TimingDisplay& timingDisplay, int objectCount, size_t wastedMemoryBytes = 0) {
			auto now = std::chrono::high_resolution_clock::now();
			double elapsedSeconds = std::chrono::duration<double>(now - startTime).count();

			const auto& timingData = timingDisplay.GetTimingData();

			PerformanceRecord record{
				elapsedSeconds,
				timingData.averageFPS,
				timingData.averageFrameTimeMs,
				timingData.averagePhysicsTimeMs,
				timingData.averageRenderTimeMs,
				objectCount,
				wastedMemoryBytes
			};

			records.push_back(record);

			// Write immediately for safety
			if (csvFile.is_open()) {
				csvFile << record.ToCSVLine() << "\n";
				csvFile.flush();  // Ensure data is written to disk
			}
		}

		void Flush() {
			if (csvFile.is_open()) {
				csvFile.flush();
			}
		}

		size_t GetRecordCount() const {
			return records.size();
		}

	private:
		std::string filename;
		std::ofstream csvFile;
		double logInterval;
		double timeSinceLastLog;
		std::vector<PerformanceRecord> records;
		std::chrono::high_resolution_clock::time_point startTime;

		void InitializeFile() {
			csvFile.open(filename, std::ios::app);  // Append mode to avoid overwriting
			if (csvFile.is_open()) {
				// Check if file is empty
				csvFile.seekp(0, std::ios::end);
				std::streampos fileSize = csvFile.tellp();

				if (fileSize == 0) {
					// File is empty, write header with wasted memory column
					csvFile << "Elapsed_Time_s,Avg_FPS,Avg_Frame_Time_ms,Avg_Physics_Time_ms,Avg_Render_Time_ms,Object_Count,Wasted_Memory_MB\n";
					csvFile.flush();
				}
			}
		}
	};

}