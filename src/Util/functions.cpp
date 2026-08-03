#include <string>
#include <iomanip>
#include <array>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>

#include <glm/gtc/type_ptr.hpp>

#include <vel/Util/functions.h>


namespace vel
{

	bool isPowerOfTwo(int n)
	{
		if (n == 0)
			return false;

		return (ceil(log2(n)) == floor(log2(n)));
	}

	std::string str_replace(const std::string& from, const std::string& to, std::string str)
	{
		if (from.empty())
			return str;

		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}

		return str;
	}

	btQuaternion glmToBulletQuat(glm::quat glmQuat)
	{
		return btQuaternion(glmQuat.x, glmQuat.y, glmQuat.z, glmQuat.w);
	}

	glm::quat bulletToGlmQuat(btQuaternion btQuat)
	{
		return glm::quat(btQuat.getW(),
			btQuat.getX(),
			btQuat.getY(),
			btQuat.getZ()
		);
	}

    std::vector<std::string> explode_string(std::string const & s, char delim)
    {
        std::vector<std::string> result;
        std::istringstream iss(s);

        for (std::string token; std::getline(iss, token, delim); )
        {
            result.push_back(std::move(token));
        }

        return result;
    }

    std::string char_to_string(char* a)
    {
        std::string s = "";
        for (int i = 0; i < (sizeof(a) / sizeof(char)); i++) {
            s = s + a[i];
        }
        return s;
    }

	bool sin_vector(std::string needle, std::vector<std::string> haystack)
	{
		if (std::find(haystack.begin(), haystack.end(), needle) != haystack.end())
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool string_contains(std::string needle, std::string haystack)
	{
		if (haystack.find(needle) != std::string::npos)
		{
			return true;
		}
		return false;
	}

	bool approximatelyEqual(float a, float b, float epsilon)
	{
		return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
	}

	bool essentiallyEqual(float a, float b, float epsilon)
	{
		return fabs(a - b) <= ((fabs(a) > fabs(b) ? fabs(b) : fabs(a)) * epsilon);
	}

	bool definitelyGreaterThan(float a, float b, float epsilon)
	{
		return (a - b) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
	}

	bool definitelyLessThan(float a, float b, float epsilon)
	{
		return (b - a) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
	}

	bool sortOfEquals(const float a, const float b, const float tolerance)
	{
		return (a + tolerance >= b) && (a - tolerance <= b);
	}

	btVector3 glmToBulletVec3(glm::vec3 glmVec)
	{
		return btVector3(glmVec.x, glmVec.y, glmVec.z);
	}

	glm::vec3 bulletToGlmVec3(btVector3 btVec)
	{
		return glm::vec3(btVec.getX(), btVec.getY(), btVec.getZ());
	}

	glm::vec2 invertVec2(glm::vec2 in)
	{
		return glm::vec2(-in.x, -in.y);
	}

	glm::mat4 bulletTransformToGlmMat4(btTransform t)
	{
		glm::mat4 out;
		t.getOpenGLMatrix(glm::value_ptr(out));
		return out;
	}

	btMatrix3x3 glmMat3ToBulletMat3(const glm::mat3& m) 
	{ 
		return btMatrix3x3(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2]); 
	}


	btTransform glmMat4ToBulletTransform(const glm::mat4& m)
	{
		glm::mat3 m3(m);
		return btTransform(glmMat3ToBulletMat3(m3), glmToBulletVec3(glm::vec3(m[3][0], m[3][1], m[3][2])));
	}

	//https://stackoverflow.com/questions/4353525/floating-point-linear-interpolation
	float lerpf(float a, float b, float f)
	{
		return (a * (1.0 - f)) + (b * f);
	}

	bool randomFiftyFifty()
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> distrib(0, 1);

		return distrib(gen) == 1;
	}

	glm::quat calculateRotation(const glm::vec3& from, const glm::vec3& to)
	{
		glm::vec3 f = glm::normalize(from);
		glm::vec3 t = glm::normalize(to);

		// Check if the vectors are almost parallel
		if (glm::length(glm::cross(f, t)) < 1e-6) 
		{
			// If they are almost parallel, the rotation is zero or 180 degrees
			if (glm::dot(f, t) > 0.99999f) 
			{
				// Zero rotation
				return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
			}
			else 
			{
				// 180 degree rotation around an orthogonal axis
				glm::vec3 orthogonal = glm::abs(f.x) > glm::abs(f.z) ? glm::vec3(-f.y, f.x, 0.0f) : glm::vec3(0.0f, -f.z, f.y);
				orthogonal = glm::normalize(orthogonal);
				return glm::angleAxis(glm::pi<float>(), orthogonal);
			}
		}

		// Calculate the rotation axis and angle
		glm::vec3 axis = glm::normalize(glm::cross(f, t));
		float angle = glm::acos(glm::dot(f, t));

		// Create the quaternion from axis and angle
		glm::quat rotation = glm::angleAxis(angle, axis);

		return rotation;
	}

	glm::mat4 ozzFloat4x4ToGlmMat4(const ozz::math::Float4x4& in)
	{
		glm::mat4 out;

		ozz::math::StorePtrU(in.cols[0], &out[0][0]);
		ozz::math::StorePtrU(in.cols[1], &out[1][0]);
		ozz::math::StorePtrU(in.cols[2], &out[2][0]);
		ozz::math::StorePtrU(in.cols[3], &out[3][0]);

		return out;
	}

	void quatToPitchYawRad(const glm::quat& q, float& outPitchRad, float& outYawRad)
	{
		// Rotate a canonical forward vector by the orientation.
		// GLM convention: forward is -Z.
		glm::vec3 fwd = q * glm::vec3(0.0f, 0.0f, -1.0f);

		// Yaw: angle around world up, based on XZ projection of forward.
		// yaw = atan2(fwd.x, -fwd.z) for forward=-Z convention.
		outYawRad = std::atan2(fwd.x, -fwd.z);

		// Pitch: angle up/down. Clamp to avoid NaNs from numerical drift.
		float fy = glm::clamp(fwd.y, -1.0f, 1.0f);
		outPitchRad = std::asin(fy);

		// Note: This pitch is in [-pi/2, pi/2], which matches typical FPS pitch limits.
		// IE: [-90, +90]
	}

	std::string generateUUID()
	{
		static thread_local std::mt19937_64 rng{ std::random_device{}() };

		std::array<uint8_t, 16> bytes{};

		for (size_t i = 0; i < bytes.size(); i += 8)
		{
			uint64_t value = rng();

			for (size_t j = 0; j < 8 && (i + j) < bytes.size(); ++j)
			{
				bytes[i + j] = static_cast<uint8_t>((value >> (j * 8)) & 0xFF);
			}
		}

		// UUID version 4
		bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0F) | 0x40);

		// UUID variant: 10xxxxxx
		bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3F) | 0x80);

		std::ostringstream oss;
		oss << std::hex << std::setfill('0');

		for (size_t i = 0; i < bytes.size(); ++i)
		{
			oss << std::setw(2) << static_cast<int>(bytes[i]);

			if (i == 3 || i == 5 || i == 7 || i == 9)
				oss << '-';
		}

		return oss.str();
	}

}