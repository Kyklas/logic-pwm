#include <cmath>

#include "PWMAnalyzer.h"
#include "PWMAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

PWMAnalyzer::PWMAnalyzer()
    :   Analyzer2(),
        mSettings(new PWMAnalyzerSettings()),
        mSimulationInitilized(false),
        mSampleRateHz(0)
{
    SetAnalyzerSettings(mSettings.get());
}

PWMAnalyzer::~PWMAnalyzer()
{
    KillThread();
}

void PWMAnalyzer::SetupResults()
{
    mResults.reset(new PWMAnalyzerResults(this, mSettings.get()));
    SetAnalyzerResults(mResults.get());
    mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannel);
}

void PWMAnalyzer::WorkerThread()
{
    mSampleRateHz = GetSampleRate();
    mPWM = GetAnalyzerChannelData(mSettings->mInputChannel);

	double SampleQuantumUS = 1000000.0f / (double)mSampleRateHz;

    // Find the next full pulse for a clean start.
    mPWM->AdvanceToNextEdge();
    if (mPWM->GetBitState() == BIT_LOW) {
        mPWM->AdvanceToNextEdge();
    }
	U64 px_number=0;

    int type = mSettings->mAnalysisType;

    double prevval(0);
	U64 px_time[25][3] = {0};
	U64 px_val;
	U8 frametype;
	double pulse_time_us;
	int px_idx;
	U64 latch_start, latch_end;
    for (;;) {

		latch_end = latch_start = -1;
		px_idx = 0; frametype = 1; px_val = 0;

		do
		{
			px_time[px_idx][0] = mPWM->GetSampleNumber(); // Rising
			mPWM->AdvanceToNextEdge();
			px_time[px_idx][1] = mPWM->GetSampleNumber(); // Falling

			// Processing Hi pulse

			pulse_time_us = SamplesToUs(px_time[px_idx][1] - px_time[px_idx][0]);
			if (0.2f <= pulse_time_us && pulse_time_us <= 0.5f)
			{
				// 0 code
				px_val <<= 1;
			}
			else if (0.55f <= pulse_time_us && pulse_time_us <= 5.5f)
			{
				// 1 code
				px_val <<= 1;
				px_val |= 1;
			}
			else
			{
				// this is not a valid pulse
				mResults->AddMarker((px_time[px_idx][1] + px_time[px_idx][0])/2,
					AnalyzerResults::ErrorX,
					mSettings->mInputChannel);

				// Move on the Data channel
				mPWM->AdvanceToNextEdge();

				if(px_idx == 0)
				{
					continue;
				}
				else
				{
					px_time[px_idx][2] = px_time[px_idx][1];
					break;
				}
			}

			mResults->AddMarker(px_time[px_idx][0],
				AnalyzerResults::UpArrow, mSettings->mInputChannel);

		//	mResults->AddMarker(px_time[px_idx][1],
		//		AnalyzerResults::DownArrow, mSettings->mInputChannel);

			// check Low pulse
			px_time[px_idx][2] = mPWM->GetSampleOfNextEdge(); // rising
			pulse_time_us = SamplesToUs(px_time[px_idx][2] - px_time[px_idx][1]);
			if (0.45f <= pulse_time_us && pulse_time_us <= 5.0f)
			{
				// data

				// Move on the Data channel
				mPWM->AdvanceToNextEdge();
			}
			else if (0.6f <= pulse_time_us)
			{
				// data and latch

				px_time[px_idx][2] = px_time[px_idx][1] +  (U64)(0.6f / SampleQuantumUS);

				latch_start = px_time[px_idx][2];
				mPWM->AdvanceToNextEdge();
				latch_end = mPWM->GetSampleNumber(); // rising

				break;
			}
			else
			{				
				// this low pulse is not valid
				// this is not a valid pulse
				mResults->AddMarker((px_time[px_idx][2] + px_time[px_idx][1]) / 2,
					AnalyzerResults::ErrorSquare,
					mSettings->mInputChannel);




				// XXX for now
				mPWM->AdvanceToNextEdge();
			}
			if (px_idx == 23)
				break;
			px_idx++;
		} while (1);

		Frame frame;
		frame.mType = 1;
		frame.mData1 = px_val;
		frame.mData2 = px_number++;
		frame.mStartingSampleInclusive = px_time[0][0];
		frame.mEndingSampleInclusive = px_time[px_idx][2];
		mResults->AddFrame(frame);
		mResults->CommitResults();
		ReportProgress(frame.mEndingSampleInclusive);

		if (latch_end != latch_start && latch_start != -1)
		{
			px_number = 0;
			Frame frame;
			frame.mType = 0;
			frame.mStartingSampleInclusive = latch_start;
			frame.mEndingSampleInclusive = latch_end;
			mResults->AddFrame(frame);
			mResults->CommitResults();
			ReportProgress(frame.mEndingSampleInclusive);
		}



    }
}




bool PWMAnalyzer::NeedsRerun()
{
    return false;
}

U32 PWMAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor **simulation_channels)
{
    if (mSimulationInitilized == false) {
        mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 PWMAnalyzer::GetMinimumSampleRateHz()
{
    return 1000000;
}

const char *PWMAnalyzer::GetAnalyzerName() const
{
    return "PWM";
}

const char *GetAnalyzerName()
{
    return "PWM";
}

Analyzer *CreateAnalyzer()
{
    return new PWMAnalyzer();
}

void DestroyAnalyzer(Analyzer *analyzer)
{
    delete analyzer;
}

double PWMAnalyzer::SamplesToUs(U64 samples)
{
    return ((double)samples * 1000000.0) / (double)mSampleRateHz;
}

double PWMAnalyzer::Value(U64 start, U64 mid, U64 end)
{
    return DutyCycle(start, mid, end);
}
