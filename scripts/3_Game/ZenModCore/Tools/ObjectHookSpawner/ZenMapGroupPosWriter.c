class ZenMapGroupPosWriter
{
	static const string FILE_PATH = "$profile:mapgrouppos_new_entries.xml";

	static void Clear()
	{
		// overwrite/create empty file
		FileHandle fh = OpenFile(FILE_PATH, FileMode.WRITE);
		if (fh != 0)
			CloseFile(fh);
	}

	static void Append(string line)
	{
		FileHandle fh = OpenFile(FILE_PATH, FileMode.APPEND);
		if (fh == 0)
			return;

		FPrintln(fh, line);
		CloseFile(fh);
	}
}

class ZenMapGroupPosMath
{
	// mapgrouppos uses rpy="Roll Pitch Yaw" (community info; order sometimes debated)
	// DayZ object orientation is (Yaw, Pitch, Roll) in degrees.
	static vector YPR_To_RPY(vector ypr)
	{
		ypr = ypr.GetRelAngles();                 // normalizes each component into [-180,180] 
		return Vector(ypr[2], ypr[1], ypr[0]).GetRelAngles();
	}

	// a = (90 - yaw), then wrapped into [-180,180]
	static float NormalizeAngle180(float a)
	{
		while (a > 180.0)  a -= 360.0;
		while (a < -180.0) a += 360.0;
		return a;
	}

	static float Yaw_To_A(float yawDeg)
	{
		return NormalizeAngle180(90.0 - yawDeg);
	}

	static string BuildGroupLine(string groupName, vector pos, vector yprOri)
	{
		vector rpy = YPR_To_RPY(yprOri);
		float a = Yaw_To_A(rpy[2]); // yaw

		return string.Format("\t<group name=\"%1\" pos=\"%2\" rpy=\"%3\" a=\"%4\"/>",
			groupName,
			pos.ToString(false),
			rpy.ToString(false),
			a);
	}
}
