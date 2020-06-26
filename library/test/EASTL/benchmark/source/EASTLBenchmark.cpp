/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include "EASTLBenchmark.h"
#include "EASTLTest.h"
#include <string>
#include <EAMain/EAMain.h>
#include <EASTL/internal/config.h>

#ifdef _MSC_VER
	#pragma warning(push, 0)
#endif
#include <stdio.h>
#include <math.h>
#include <float.h>
#ifdef _MSC_VER
	#pragma warning(pop)
#endif



namespace Benchmark
{
	static int64_t ConvertStopwatchUnits(EA::StdC::Stopwatch::Units unitsSource, int64_t valueSource, EA::StdC::Stopwatch::Units unitsDest)
	{
		using namespace EA::StdC;

		int64_t valueDest = valueSource;

		if(unitsSource != unitsDest)
		{
			double sourceMultiplier;

			switch (unitsSource)
			{
				case Stopwatch::kUnitsCPUCycles:
					sourceMultiplier = Stopwatch::GetUnitsPerCPUCycle(unitsDest); // This will typically be a number less than 1.
					valueDest = (int64_t)(valueSource * sourceMultiplier);
					break;

				case Stopwatch::kUnitsCycles:
					sourceMultiplier = Stopwatch::GetUnitsPerStopwatchCycle(unitsDest); // This will typically be a number less than 1.
					valueDest = (int64_t)(valueSource * sourceMultiplier);
					break;

				case Stopwatch::kUnitsNanoseconds:
				case Stopwatch::kUnitsMicroseconds:
				case Stopwatch::kUnitsMilliseconds:
				case Stopwatch::kUnitsSeconds:
				case Stopwatch::kUnitsMinutes:
				case Stopwatch::kUnitsUserDefined:
					// To do. Also, handle the case of unitsDest being Cycles or CPUCycles and unitsSource being a time.
					break;
			}
		}

		return valueDest;
	}

	std::string WriteTime(int64_t timeNS)
	{
		// if(timeNS > 1000000000)
		// 	sTime.sprintf(" %6.2f s",  (double)timeNS / 1000000000);
		// else if(timeNS > 1000000)
		// 	sTime.sprintf("%6.1f ms", (double)timeNS / 1000000);
		// else if(timeNS > 1000)
		// 	sTime.sprintf("%6.1f us", (double)timeNS / 1000);
		// else
		// 	sTime.sprintf("%6.1f ns", (double)timeNS / 1);

		char buff[16] = {0};
		int sz = 0;
		if(timeNS > 1000000000)
			sz = snprintf(buff, sizeof(buff), " %6.2f s",  (double)timeNS / 1000000000);
		else if(timeNS > 1000000)
			sz = snprintf(buff, sizeof(buff), "%6.1f ms", (double)timeNS / 1000000);
		else if(timeNS > 1000)
			sz = snprintf(buff, sizeof(buff), "%6.1f us", (double)timeNS / 1000);
		else
			sz = snprintf(buff, sizeof(buff), "%6.1f ns", (double)timeNS / 1);

		return sz > 0 && sz < static_cast<int>(sizeof(buff)) ? std::string(buff) : std::string("error");
	}



	Environment gEnvironment;

	Environment& GetEnvironment()
	{
		return gEnvironment;
	}



	ResultSet gResultSet;

	ResultSet& GetResultSet()
	{
		return gResultSet;
	}



	// Scratch sprintf buffer
	char gScratchBuffer[1024];


	void DoNothing(...)
	{
		// Intentionally nothing.
	}


	void AddResult(const char* pName, int units, int64_t nTime1, int64_t nTime2, const char* pNotes)
	{
		Result result;

		result.msName   = pName;
		result.mUnits   = units;
		result.mTime1   = nTime1;
		// result.mTime1NS = ConvertStopwatchUnits((EA::StdC::Stopwatch::Units)units, nTime1, EA::StdC::Stopwatch::kUnitsNanoseconds);
		result.mTime2   = nTime2;
		// result.mTime2NS = ConvertStopwatchUnits((EA::StdC::Stopwatch::Units)units, nTime2, EA::StdC::Stopwatch::kUnitsNanoseconds);
		result.mTime3   = nTime2;
		result.mTime4   = nTime2;

		if(pNotes)
			result.msNotes = pNotes;

		gResultSet.insert(std::pair<std::string, Result>(pName, result));
	}

	void AddResult(const char* pName, int index, const EA::StdC::Stopwatch& sw, const char* pNotes)
	{
		if(index == 1) {
			Result result;

			result.msName   = pName;
			//TODO assert all units are the same
			result.mUnits   = sw.GetUnits();
			result.mTime1   = sw.GetElapsedTime();

			if(pNotes)
				result.msNotes = pNotes;

			gResultSet.insert(std::pair<std::string, Result>(pName, result));
			return;
		}


		auto result = gResultSet.find(pName);
		if(result == gResultSet.end())
			return;

		if(index == 2)
			result->second.mTime2 = sw.GetElapsedTime();
		else if(index == 3)
			result->second.mTime3 = sw.GetElapsedTime();
		else if(index == 4)
			result->second.mTime4 = sw.GetElapsedTime();
	}


	void PrintResultLine(const Result& result)
	{
		const double fRatio2         = (double)result.mTime1 / (double)result.mTime2;
		const double fRatioPrinted2  = (fRatio2 > 100) ? 100 : fRatio2;
		// const double fPercentChange = fabs(((double)result.mTime1 - (double)result.mTime2) / (((double)result.mTime1 + (double)result.mTime2) / 2));
		// const bool   bDifference    = (result.mTime1 > 10) && (result.mTime2 > 10) && (fPercentChange > 0.25);
		// const char*  pDifference    = (bDifference ? (result.mTime1 < result.mTime2 ? "-" : "+") : "");
		const char*  pDifference2    = "";


		const double fRatio3         = (double)result.mTime1 / (double)result.mTime3;
		const double fRatioPrinted3  = (fRatio3 > 100) ? 100 : fRatio3;
		const char*  pDifference3    = "";

		const double fRatio4         = (double)result.mTime1 / (double)result.mTime4;
		const double fRatioPrinted4  = (fRatio4 > 100) ? 100 : fRatio4;
		const char*  pDifference4    = ""; 




		int64_t Time1NS = ConvertStopwatchUnits((EA::StdC::Stopwatch::Units)result.mUnits, result.mTime1, EA::StdC::Stopwatch::kUnitsNanoseconds);
		std::string sClockTime1 = WriteTime(Time1NS);  // This converts an integer in nanoseconds (e.g. 23400000) to a string (e.g. "23.4 ms")

		// EA::UnitTest::Report("%-43s | %13" PRIu64 " %s | %13" PRIu64 " %s | %10.2f%10s", result.msName.c_str(), result.mTime1, sClockTime1.c_str(), result.mTime2, sClockTime2.c_str(), fRatioPrinted, pDifference);
		EA::UnitTest::Report("%-50s | %10" PRIu64 " %s | %6.2f%5s | %6.2f%5s | %6.2f%5s |", result.msName.c_str(), result.mTime1, sClockTime1.c_str(), fRatioPrinted2, pDifference2, fRatioPrinted3, pDifference3, fRatioPrinted4, pDifference4);

		if(result.msNotes.length()) // If there are any notes...
			EA::UnitTest::Report("   %s", result.msNotes.c_str());
		EA::UnitTest::Report("\n");
	}


	#if defined(EASTL_BENCHMARK_WRITE_FILE) && EASTL_BENCHMARK_WRITE_FILE

		#if !defined(EASTL_BENCHMARK_WRITE_FILE_PATH)
			#define EASTL_BENCHMARK_WRITE_FILE_PATH "BenchmarkResults.txt"
		#endif

		struct FileWriter
		{
			FILE* mpReportFile;
			EA::EAMain::ReportFunction mpSavedReportFunction;
			static FileWriter* gpFileWriter;

			static void StaticPrintfReportFunction(const char8_t* pText)
			{
				if(gpFileWriter)
					gpFileWriter->PrintfReportFunction(pText);
			}

			void PrintfReportFunction(const char8_t* pText)
			{
				fwrite(pText, strlen(pText), 1, mpReportFile);
				EA::EAMain::ReportFunction gpReportFunction = EA::EAMain::GetDefaultReportFunction();
				gpReportFunction(pText);
			}

			FileWriter() : mpReportFile(NULL), mpSavedReportFunction(NULL)
			{
				mpReportFile = fopen(EASTL_BENCHMARK_WRITE_FILE_PATH, "w+");

				if(mpReportFile)
				{
					gpFileWriter = this;
					mpSavedReportFunction = EA::EAMain::GetDefaultReportFunction();
					EA::EAMain::SetReportFunction(StaticPrintfReportFunction);
				}
			}

		   ~FileWriter()
			{
				if(mpReportFile)
				{
					gpFileWriter = NULL;
					EA::EAMain::SetReportFunction(mpSavedReportFunction);
					fclose(mpReportFile);
				}
			}
		};

		FileWriter* FileWriter::gpFileWriter = NULL;
	#endif


	void PrintResults()
	{
		#if defined(EASTL_BENCHMARK_WRITE_FILE) && EASTL_BENCHMARK_WRITE_FILE
			FileWriter fileWriter; // This will auto-execute.
		#endif

		// Print the results
		EA::UnitTest::Report("\n");
		EA::UnitTest::Report("****************************************************************************************\n");
		EA::UnitTest::Report("EASTL Benchmark test results\n");
		EA::UnitTest::Report("****************************************************************************************\n");
		EA::UnitTest::Report("\n");
		// EA::UnitTest::Report("EASTL version: %s\n", EASTL_VERSION);
		EA::UnitTest::Report("Platform: %s\n", gEnvironment.msPlatform.c_str());
		EA::UnitTest::Report("Compiler: %s\n", EA_COMPILER_STRING);
//		EA::UnitTest::Report("Allocator: Node.cpp - IibMalloc.\n");
		#if defined(EA_DEBUG) || defined(_DEBUG)
		EA::UnitTest::Report("Build: Debug.\n");
		#else
		EA::UnitTest::Report("Build: Full optimization.\n");
		#endif
		EA::UnitTest::Report("\n");
		EA::UnitTest::Report("First column values are ticks and time to complete tests.\n");
		EA::UnitTest::Report("Others are ratios to the first, under 1 means slower than, over 1 means faster.\n");
		EA::UnitTest::Report("\n");

		EA::UnitTest::Report("%-50s | %-20s | %-11s | %-11s | %-11s |\n", "Test", "std", "eastl", "safe_memory", "(no_checks)");
		EA::UnitTest::Report("---------------------------------------------------------------------------------------------------------------------\n");

		std::string sTestTypeLast;
		std::string sTestTypeTemp;

		for(ResultSet::iterator it = gResultSet.begin(); it != gResultSet.end(); ++it)
		{
			const Result& result = it->second;

			size_t n = result.msName.find('/');
			if(n == std::string::npos)
				n = result.msName.length();
			sTestTypeTemp.assign(result.msName, 0, n);

			if(sTestTypeTemp != sTestTypeLast) // If it looks like we are changing to a new test type... add an empty line to help readability.
			{
				if(it != gResultSet.begin())
					EA::UnitTest::Report("\n");
				sTestTypeLast = sTestTypeTemp;
			}

			PrintResultLine(result);
		}

		// We will print out a final line that has the sum of the rows printed above.
		Result resultSum;
		resultSum.msName = "sum";

		for(ResultSet::iterator its = gResultSet.begin(); its != gResultSet.end(); ++its)
		{
			const Result& resultTemp = its->second;

			EASTL_ASSERT(resultTemp.mUnits == EA::StdC::Stopwatch::kUnitsCPUCycles); // Our ConvertStopwatchUnits call below assumes that every measured time is CPUCycles.
			resultSum.mTime1 += resultTemp.mTime1;
			resultSum.mTime2 += resultTemp.mTime2;
			resultSum.mTime3 += resultTemp.mTime3;
			resultSum.mTime4 += resultTemp.mTime4;
		}

		// We do this convert as a final step instead of the loop in order to avoid loss of precision.
		// resultSum.mTime1NS = ConvertStopwatchUnits(EA::StdC::Stopwatch::kUnitsCPUCycles, resultSum.mTime1, EA::StdC::Stopwatch::kUnitsNanoseconds);
		// resultSum.mTime2NS = ConvertStopwatchUnits(EA::StdC::Stopwatch::kUnitsCPUCycles, resultSum.mTime2, EA::StdC::Stopwatch::kUnitsNanoseconds);
		EA::UnitTest::Report("\n");
		PrintResultLine(resultSum);

		EA::UnitTest::Report("\n");
		EA::UnitTest::Report("****************************************************************************************\n");
		EA::UnitTest::Report("\n");

		// Clear the results
		gResultSet.clear();
		gEnvironment.clear();
	}

} // namespace Benchmark



















