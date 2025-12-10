#include "Debug.h"
using namespace NCL;

std::vector<Debug::DebugStringEntry>	Debug::stringEntries;
std::vector<Debug::DebugLineEntry>		Debug::lineEntries;
std::vector<Debug::DebugTexEntry>		Debug::texEntries;

SimpleFont* Debug::debugFont = nullptr;

const Vector4 Debug::RED		= Vector4(1, 0, 0, 1);
const Vector4 Debug::GREEN		= Vector4(0, 1, 0, 1);
const Vector4 Debug::BLUE		= Vector4(0, 0, 1, 1);

const Vector4 Debug::BLACK		= Vector4(0, 0, 0, 1);
const Vector4 Debug::WHITE		= Vector4(1, 1, 1, 1);

const Vector4 Debug::YELLOW		= Vector4(1, 1, 0, 1);
const Vector4 Debug::MAGENTA	= Vector4(1, 0, 1, 1);
const Vector4 Debug::CYAN		= Vector4(0, 1, 1, 1);

void Debug::Print(const std::string& text, const Vector2& pos, const Vector4& colour) {
	DebugStringEntry newEntry;

	newEntry.data = text;
	newEntry.position = pos;
	newEntry.colour = colour;

	stringEntries.emplace_back(newEntry);
}

void Debug::DrawLine(const Vector3& startpoint, const Vector3& endpoint, const Vector4& colour, float time) {
	DebugLineEntry newEntry;

	newEntry.start = startpoint;
	newEntry.end = endpoint;
	newEntry.colourA = colour;
	newEntry.colourB = colour;
	newEntry.time = time;

	lineEntries.emplace_back(newEntry);
}

void Debug::DrawTex(const Texture& t, const Vector2& pos, const Vector2& scale, const Vector4& colour) {
	DebugTexEntry newEntry;

	newEntry.t			= &t;
	newEntry.position	= pos;
	newEntry.scale		= scale;
	newEntry.colour		= colour;

	texEntries.push_back(newEntry);
}

void Debug::DrawAxisLines(const Matrix4& modelMatrix, float scaleBoost, float time) {
	Matrix4 local = modelMatrix;
	local.SetColumn(3, Vector4(0,0,0,1));

	Vector3 fwd = local * Vector4(0, 0, -1, 1.0f);
	Vector3 up = local * Vector4(0, 1, 0, 1.0f);
	Vector3 right = local * Vector4(1, 0, 0, 1.0f);

	Vector3 worldPos = modelMatrix.GetColumn(3);

	DrawLine(worldPos, worldPos + (right * scaleBoost), Debug::RED, time);
	DrawLine(worldPos, worldPos + (up * scaleBoost), Debug::GREEN, time);
	DrawLine(worldPos, worldPos + (fwd * scaleBoost), Debug::BLUE, time);
}

void NCL::Debug::debugDrawAABBs(const Vector3& center, const Vector3& halfSizes, const Vector4& colour, float time)
{
	// Compute min/max corners
	const Vector3 min = center - halfSizes;
	const Vector3 max = center + halfSizes;

	// 8 corners
	const Vector3 c[8] = {
		Vector3(min.x, min.y, min.z), // 0
		Vector3(min.x, min.y, max.z), // 1
		Vector3(min.x, max.y, min.z), // 2
		Vector3(min.x, max.y, max.z), // 3
		Vector3(max.x, min.y, min.z), // 4
		Vector3(max.x, min.y, max.z), // 5
		Vector3(max.x, max.y, min.z), // 6
		Vector3(max.x, max.y, max.z)  // 7
	};

	// 12 edges (pairs of corner indices)
	const int edges[12][2] = {
		{0,1}, {0,2}, {0,4},
		{7,5}, {7,6}, {7,3},
		{1,3}, {1,5},
		{2,3}, {2,6},
		{4,5}, {4,6}
	};

	for (const auto& e : edges) {
		DrawLine(c[e[0]], c[e[1]], colour, time);
	}
}

void NCL::Debug::debugDrawSphere(const Vector3& center, float radius, const Vector4& colour, float time, int segments)
{
	if (segments < 8) segments = 8; // ensure reasonable tessellation
	const float twoPi = 6.28318530717958647692f;

	// Helper: closed polyline (ring) approximated by 'segments' straight lines
	auto drawRing = [&](auto pointAtAngle) {
		Vector3 prev = pointAtAngle(0.0f);
		for (int i = 1; i <= segments; ++i) {
			float a = (twoPi * i) / static_cast<float>(segments);
			Vector3 curr = pointAtAngle(a);
			DrawLine(prev, curr, colour, time);
			prev = curr;
		}
		};

	// XY plane ring
	drawRing([&](float a) {
		return center + Vector3(std::cos(a) * radius, std::sin(a) * radius, 0.0f);
		});
	// XZ plane ring
	drawRing([&](float a) {
		return center + Vector3(std::cos(a) * radius, 0.0f, std::sin(a) * radius);
		});
	// YZ plane ring
	drawRing([&](float a) {
		return center + Vector3(0.0f, std::cos(a) * radius, std::sin(a) * radius);
		});
}

void Debug::UpdateRenderables(float dt) {
	int trim = 0;
	for (int i = 0; i < lineEntries.size(); ) {
		DebugLineEntry* e = &lineEntries[i];
		e->time -= dt;
		if (e->time < 0) {
			trim++;
			lineEntries[i] = lineEntries[lineEntries.size() - trim];
		}
		else {
			++i;
		}
		if (i + trim >= lineEntries.size()) {
			break;
		}
	}
	lineEntries.resize(lineEntries.size() - trim);
	stringEntries.clear();
	texEntries.clear();
}

SimpleFont* Debug::GetDebugFont() {
	return debugFont;
}

void Debug::CreateDebugFont(const std::string& dataFile, Texture& tex) {
	debugFont = new SimpleFont(dataFile, tex);
}

const std::vector<Debug::DebugStringEntry>& Debug::GetDebugStrings() {
	return stringEntries;
}

const std::vector<Debug::DebugLineEntry>& Debug::GetDebugLines() {
	return lineEntries;
}

const std::vector<Debug::DebugTexEntry>& Debug::GetDebugTex() {
	return texEntries;
}