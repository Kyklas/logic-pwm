#include "PWMAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "PWMAnalyzer.h"
#include "PWMAnalyzerSettings.h"
#include <fstream>

PWMAnalyzerResults::PWMAnalyzerResults(PWMAnalyzer *analyzer, PWMAnalyzerSettings *settings)
    :   AnalyzerResults(),
        mSettings(settings),
        mAnalyzer(analyzer)
{
}

PWMAnalyzerResults::~PWMAnalyzerResults()
{
}

double PWMAnalyzerResults::DutyCycle(Frame frame)
{
    return mAnalyzer->DutyCycle(frame.mStartingSampleInclusive, frame.mData1,
                                frame.mEndingSampleInclusive);
}

double PWMAnalyzerResults::Width(Frame frame)
{
    return mAnalyzer->Width(frame.mStartingSampleInclusive, frame.mData1);
}

void PWMAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel &channel, DisplayBase display_base)
{
    ClearResultStrings();
    Frame frame = GetFrame(frame_index);

    char number_str[128] = {0};
    char delta_str[128] = {0};
    char *units = NULL;

	if (frame.mType == 0)
	{
		AddResultString("RET");
	}
	else
	{
		snprintf(number_str, sizeof(number_str), "%d-0x%06X%s",frame.mData2,frame.mData1, (frame.mType==1)?"":" *");
		AddResultString(number_str);
	}
}

void PWMAnalyzerResults::GenerateExportFile(const char *file, DisplayBase display_base, U32 export_type_user_id)
{

    std::ofstream file_stream(file, std::ios::out);

    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();

    file_stream << "Time [s],High,Low,Duty,Frequency" << std::endl;

    U64 num_frames = GetNumFrames();
    for (U32 i = 0; i < num_frames; i++) {
        Frame frame = GetFrame(i);

        char time_str[128];
        AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);

        char number_str[128];
        snprintf(number_str, sizeof(number_str), "%f,%f,%f,%.1f",
                 Width(frame),
                 mAnalyzer->Width(frame.mData1, frame.mEndingSampleInclusive),
                 DutyCycle(frame),
                 1000000.0 / mAnalyzer->Width(frame.mStartingSampleInclusive, frame.mEndingSampleInclusive));

        file_stream << time_str << "," << number_str << std::endl;

        if (UpdateExportProgressAndCheckForCancel(i, num_frames) == true) {
            file_stream.close();
            return;
        }
    }

    file_stream.close();
}

void PWMAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
	//return;
    Frame frame = GetFrame(frame_index);
    ClearTabularText();

    char number_str[128];
    char delta_str[128] = {0};
    char *units = NULL;
	if (frame.mType == 0)
	{
		AddTabularText("RET");
	}
	else
	{
		//if (frame_index < 1 || GetFrame(frame_index-1).mData1 != frame.mData1)
		if(frame.mType == 2)
		{
			snprintf(number_str, sizeof(number_str), "%d-0x%06X", frame.mData2, frame.mData1);
			AddTabularText(number_str);
		}
	}
}

void PWMAnalyzerResults::FillDelta(U64 frame_index, char *b, size_t len)
{
	return;

    if (frame_index == 0) {
        return;
    }

    Frame current = GetFrame(frame_index);
    Frame prev = GetFrame(frame_index-1);

    double val = mAnalyzer->Value(current.mStartingSampleInclusive, current.mData1,
                                  current.mEndingSampleInclusive);
    double prevval = mAnalyzer->Value(prev.mStartingSampleInclusive, prev.mData1,
                                      prev.mEndingSampleInclusive);

    _snprintf(b, len, " (%s%.0f)", (val-prevval > 0 ? "+" : ""), val-prevval);
	//_snprintf(b, len, " (%s%.0f)","kyklas");
}

void PWMAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base)
{
    ClearResultStrings();
    AddResultString("not supported");
}

void PWMAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base)
{
    ClearResultStrings();
    AddResultString("not supported");
}
