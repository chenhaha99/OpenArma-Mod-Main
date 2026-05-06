// OA_Config.c — Runtime configuration for OpenArma mod.
// Only ApiUrl and ApiKey are set locally. All other config comes from backend.

class OA_Log
{
	protected static FileHandle s_hFile;
	protected static string s_sCurrentDate;

	static void Log(string msg)
	{
		int hour, minute, second;
		System.GetHourMinuteSecondUTC(hour, minute, second);

		string sH = hour.ToString();
		string sM = minute.ToString();
		string sS = second.ToString();
		if (hour < 10) sH = "0" + sH;
		if (minute < 10) sM = "0" + sM;
		if (second < 10) sS = "0" + sS;

		string full = "[" + sH + ":" + sM + ":" + sS + " UTC] " + msg;
		Print(full);

		_WriteToFile(full);
	}

	static void Close()
	{
		if (s_hFile)
		{
			s_hFile.Close();
			s_hFile = null;
		}
		s_sCurrentDate = "";
	}

	protected static void _WriteToFile(string line)
	{
		int year, month, day;
		System.GetYearMonthDayUTC(year, month, day);

		string sY = year.ToString();
		string sM = month.ToString();
		string sD = day.ToString();
		if (month < 10) sM = "0" + sM;
		if (day < 10) sD = "0" + sD;
		string dateStr = sY + "-" + sM + "-" + sD;

		if (s_sCurrentDate != dateStr)
		{
			if (s_hFile)
			{
				s_hFile.Close();
				s_hFile = null;
			}
			s_sCurrentDate = dateStr;
		}

		if (!s_hFile)
		{
			string path = "$profile:OA_Log_" + dateStr + ".log";
			s_hFile = FileIO.OpenFile(path, FileMode.APPEND);
		}

		if (s_hFile)
			s_hFile.WriteLine(line);
	}

	static string GridRef(vector pos)
	{
		return GridRefXZ(pos[0], pos[2]);
	}

	static string GridRefXZ(float x, float z)
	{
		int hE = (x * 0.01);
		int hN = (z * 0.01);
		string sE = hE.ToString();
		string sN = hN.ToString();
		while (sE.Length() < 3) sE = "0" + sE;
		while (sN.Length() < 3) sN = "0" + sN;
		return sE + " " + sN;
	}

	static string StripLocKey(string raw)
	{
		if (raw.Length() < 5)
			return raw;

		if (raw.Substring(0, 1) != "#")
			return raw;

		int lastUnderscore = -1;
		int len = raw.Length();
		for (int i = len - 1; i >= 0; i--)
		{
			if (raw.Substring(i, 1) == "_")
			{
				lastUnderscore = i;
				break;
			}
		}
		if (lastUnderscore > 0 && lastUnderscore < len - 1)
			return raw.Substring(lastUnderscore + 1, len - lastUnderscore - 1);

		return raw;
	}

	static string StripWeaponLocKey(string raw)
	{
		if (raw.Length() < 5)
			return raw;

		if (raw.Substring(0, 1) != "#")
			return raw;

		if (raw.Length() > 11 && raw.Substring(0, 11) == "#AR-Weapon_")
		{
			string mid = raw.Substring(11, raw.Length() - 11);
			if (mid.Length() > 5)
			{
				string suffix = mid.Substring(mid.Length() - 5, 5);
				if (suffix == "_Name")
					return mid.Substring(0, mid.Length() - 5);
			}
			return mid;
		}

		return StripLocKey(raw);
	}
}

class OA_Config
{
	string ApiUrl = "https://openarma.com";
	string ApiKey = "";

	float DecisionInterval = 30.0;
	bool EmergencyDetection = true;
	int MaxSquads = 12;
	string GameMode = "game_master";
	string Language = "en";

	bool IsConfigured()
	{
		return ApiUrl != "" && ApiKey != "";
	}
}
