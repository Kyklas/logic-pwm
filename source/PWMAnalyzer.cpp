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

	double quantumUS = 1000000.0f / (double)mSampleRateHz;

    // Find the next full pulse for a clean start.
    mPWM->AdvanceToNextEdge();
    if (mPWM->GetBitState() == BIT_LOW) {
        mPWM->AdvanceToNextEdge();
    }
	U64 px_number=0;

    int type = mSettings->mAnalysisType;

    double prevval(0);
	U64 px_time[24][3];
	U64 px_val;
	U8 frametype;

    for (;;) {


		for (int px_idx = 0, frametype=1, px_val=0; px_idx < 24 ; px_idx++)
		{
			px_time[px_idx][0] = mPWM->GetSampleNumber(); // Rising
			mPWM->AdvanceToNextEdge();
			px_time[px_idx][1] = mPWM->GetSampleNumber(); // Falling
			mPWM->AdvanceToNextEdge();
			px_time[px_idx][2] = mPWM->GetSampleNumber(); // Rising

			if ( SamplesToUs(px_time[px_idx][2] - px_time[px_idx][0]) - 1.25f  < - 0.125f )
			{
				mResults->AddMarker(px_time[px_idx][2],
					AnalyzerResults::ErrorX,
					mSettings->mInputChannel);


				frametype = 2; // error frame
				mPWM->AdvanceToNextEdge();
				//px_time[px_idx][1] = mPWM->GetSampleNumber(); // Falling
				mPWM->AdvanceToNextEdge();
				px_time[px_idx][2] = mPWM->GetSampleNumber(); // Rising
			}

			if (SamplesToUs(px_time[px_idx][2] - px_time[px_idx][1]) > 10.0f)
			{
				px_number = 0;
				Frame frame;
				frame.mType = 0;
				frame.mData2 = px_number++;
				frame.mStartingSampleInclusive = px_time[px_idx][1];
				frame.mEndingSampleInclusive = px_time[px_idx][2];
				mResults->AddFrame(frame);
				mResults->CommitResults();
				ReportProgress(frame.mEndingSampleInclusive);
				break;
			}

			if (SamplesToUs(px_time[px_idx][2] - px_time[px_idx][0]) > 2*1.25f)
			{
				break;
			}

			double val = Value(px_time[px_idx][0], px_time[px_idx][1], px_time[px_idx][2]);

			px_val |= ((val > 45.0f) ? 1 : 0) << ( 23 - px_idx );

			if (px_idx == 23)
			{
				Frame frame;
				frame.mType = frametype;
				frame.mData1 = px_val;
				frame.mData2 = px_number++;
				frame.mStartingSampleInclusive = px_time[0][0];
				frame.mEndingSampleInclusive = px_time[23][2];
				mResults->AddFrame(frame);
				mResults->CommitResults();
				ReportProgress(frame.mEndingSampleInclusive);
			}
		}


		/*
        U64 start = mPWM->GetSampleNumber(); // Rising
        mPWM->AdvanceToNextEdge();
        U64 mid = mPWM->GetSampleNumber(); // Falling
        mPWM->AdvanceToNextEdge();
        U64 end = mPWM->GetSampleNumber(); // Rising
		*/
		/*
        double val = Value(start, mid, end);
        if (std::abs(val - prevval) >= mSettings->mMinChange) {
            mResults->AddMarker(end - ((end - start) / 2),
                                val > prevval ? AnalyzerResults::UpArrow : AnalyzerResults::DownArrow,
                                mSettings->mInputChannel);

            Frame frame;
            frame.mData1 = mid;
            frame.mStartingSampleInclusive = start;
            frame.mEndingSampleInclusive = end;
            mResults->AddFrame(frame);
            mResults->CommitResults();
            ReportProgress(frame.mEndingSampleInclusive);

            prevval = val;
        }
		*/
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
