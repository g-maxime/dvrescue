/* -LICENSE-START-
 ** Copyright (c) 2010 Blackmagic Design
 **  
 ** Permission is hereby granted, free of charge, to any person or organization 
 ** obtaining a copy of the software and accompanying documentation (the 
 ** "Software") to use, reproduce, display, distribute, sub-license, execute, 
 ** and transmit the Software, and to prepare derivative works of the Software, 
 ** and to permit third-parties to whom the Software is furnished to do so, in 
 ** accordance with:
 ** 
 ** (1) if the Software is obtained from Blackmagic Design, the End User License 
 ** Agreement for the Software Development Kit (“EULA”) available at 
 ** https://www.blackmagicdesign.com/EULA/DeckLinkSDK; or
 ** 
 ** (2) if the Software is obtained from any third party, such licensing terms 
 ** as notified by that third party,
 ** 
 ** and all subject to the following:
 ** 
 ** (3) the copyright notices in the Software and this entire statement, 
 ** including the above license grant, this restriction and the following 
 ** disclaimer, must be included in all copies of the Software, in whole or in 
 ** part, and all derivative works of the Software, unless such copies or 
 ** derivative works are solely in the form of machine-executable object code 
 ** generated by a source language processor.
 ** 
 ** (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 ** OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 ** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
 ** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
 ** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
 ** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 ** DEALINGS IN THE SOFTWARE.
 ** 
 ** A copy of the Software is available free of charge at 
 ** https://www.blackmagicdesign.com/desktopvideo_sdk under the EULA.
 ** 
 ** -LICENSE-END-
 */
#include "export.h"
#include "common.h"


// settings for NTSC 29.97 - UYVY pixel format
// Adjust gFramecolours and ETTHelper::fillFrame() if a different pixel format is used.
#define BMD_DISPLAYMODE		bmdModeNTSC
#define TCISDROPFRAME		true
#define PIXEL_FMT			bmdFormat8BitYUV

// in and out points                    H H :M M :S S :F F
#define START_TC            MAKE_TC_BCD(0,1, 0,0, 0,0, 0,0)
#define STOP_TC             MAKE_TC_BCD(0,1, 0,0, 0,5, 0,0)


#define NUM_COLOURED_FRAMES	8
// array of colours used to fill in frames
static uint32_t gFrameColours[NUM_COLOURED_FRAMES] =
{
	0xeb80eb80, 0xa28ea22c, 0x832c839c, 0x703a7048,
	0x54c654b8, 0x41d44164, 0x237223d4, 0x10801080
};

// Get the buffer for an decklink video frame at the given index, and
// fill the buffer with the colour at the same index in the colour array
bool	ETTHelper::fillFrame(int index)
{
	bool		result = false;
	long		numWords = m_height * m_width * 2 / 4; // number of words in the frame
	uint32_t	*nextWord = NULL;
	
	// Make sure there is an even number of pixels
	if ((m_height * m_width * 2) % 4 != 0)
	{
		printf("Odd number of pixels in buffer !!!\n");
		goto bail;
	}
	
	// get the buffer for our frame
	if (m_videoFrames[index]->GetBytes((void **)&nextWord) != S_OK)
	{
		printf("Could not get pixel buffer\n");
		goto bail;
	}
	
	// fill in the buffer with the right colour
	while(numWords-- > 0)
		*(nextWord++) = gFrameColours[index];
		  
	result = true;

bail:
	return result;
}

// Create an array of 'NUM_COLOURED_FRAMES' IDeckLinkMutableVideoFrames
// and fill in each frame with its own colour
bool	ETTHelper::createFrames()
{
	int i;
	bool result = false;
	
	//allocate and reset frame array
	m_videoFrames = (IDeckLinkMutableVideoFrame **) malloc(NUM_COLOURED_FRAMES * sizeof(IDeckLinkMutableVideoFrame *));
	memset(m_videoFrames, 0x0, NUM_COLOURED_FRAMES * sizeof(IDeckLinkMutableVideoFrame *));
	
	// allocate IDeckLink video frames
	for(i = 0; i<NUM_COLOURED_FRAMES; i++)
	{
		if (m_deckLinkOutput->CreateVideoFrame(m_width, m_height, m_width * 2, PIXEL_FMT, bmdFrameFlagDefault, &m_videoFrames[i]) != S_OK)
		{
			printf("Could not obtain frame %d\n", (i+1));
			goto bail;
		}		
		
		// fill in frame buffer
		fillFrame(i);				  
	}
	
	result = true;
	
bail:
	if (! result)
		releaseFrames();
	
	return result;
}


// Release each decklink frame and delete the array
void	ETTHelper::releaseFrames()
{
	if (m_videoFrames)
	{
		int i;
		// release each frame
		for(i = 0; i<NUM_COLOURED_FRAMES; i++)
		{
			if (m_videoFrames[i])
			{
				m_videoFrames[i]->Release();
				m_videoFrames[i] = NULL;
			}
		}
		
		// delete the array
		free(m_videoFrames);
		m_videoFrames = NULL;
	}
}

// Schedule a single frame or a few frames if pre-rolling
bool	ETTHelper::scheduleNextFrame(bool preroll)
{
	bool result = false;
	int iter = preroll ? NUM_COLOURED_FRAMES : 1; // how many frames will we schedule ? NUM_COLOURED_FRAMES if pre-rolling, 1 otherwise
	
	while(iter-- > 0)
	{
		if (m_deckLinkOutput->ScheduleVideoFrame(m_videoFrames[m_nextFrameIndex], (m_totalFrameScheduled * m_frameDuration), m_frameDuration, m_timeScale) == S_OK)
		{
			m_nextFrameIndex = (m_nextFrameIndex == (NUM_COLOURED_FRAMES - 1)) ? 0 : m_nextFrameIndex +1;
			m_totalFrameScheduled++;
			result = true;
		}
		else
		{
			printf("Could not schedule next frame (total frame scheduled: %d)\n", m_totalFrameScheduled);
			break;
		}
	}
	
	return result;
}

// Prepare IDeckLinkOutput to output video:
// - Get frame width, height, duration, ...
// - Setup callback object and output video mode,
// - Enable video output and
// - Create output frames
bool	ETTHelper::setupDeckLinkOutput()
{
	bool							result = false;
    IDeckLinkDisplayModeIterator*	displayModeIterator = NULL;
    IDeckLinkDisplayMode*			deckLinkDisplayMode = NULL;
	
	m_width = -1;
	
	// set callback
	m_deckLinkOutput->SetScheduledFrameCompletionCallback(this);
	
	// get frame scale and duration for the video mode
    if (m_deckLinkOutput->GetDisplayModeIterator(&displayModeIterator) != S_OK)
		goto bail;
	
    while (displayModeIterator->Next(&deckLinkDisplayMode) == S_OK)
    {
		if (deckLinkDisplayMode->GetDisplayMode() == BMD_DISPLAYMODE)
		{
			m_width = deckLinkDisplayMode->GetWidth();
			m_height = deckLinkDisplayMode->GetHeight();
			deckLinkDisplayMode->GetFrameRate(&m_frameDuration, &m_timeScale);
			deckLinkDisplayMode->Release();
			deckLinkDisplayMode = NULL;

			break;
		}
		
		deckLinkDisplayMode->Release();
		deckLinkDisplayMode = NULL;
    }
	
	displayModeIterator->Release();
	displayModeIterator = NULL;
	
	if (m_width == -1)
	{
		printf("Unable to find requested video mode\n");
		goto bail;
	}
	
	// enable video output
	if (m_deckLinkOutput->EnableVideoOutput(BMD_DISPLAYMODE, bmdVideoOutputFlagDefault) != S_OK)
	{
		printf("Could not enable video output\n");
		goto bail;
	}
	
	// create coloured frames
	if (! createFrames())
		goto bail;
	
	result = true;
	
bail:
	if (! result)
	{
		// release coloured frames
		releaseFrames();
	}
	
	return result;
}

// Prepare IDeckLinkDeckControl to export to tape
bool	ETTHelper::setupDeckControl()
{
	BMDDeckControlError err;
	bool result = false;
	
	// set callback
	m_deckControl->SetCallback(this);
	
	pthread_mutex_lock(&m_mutex);
		// open connection to deck
		if (m_deckControl->Open(m_timeScale, m_frameDuration, TCISDROPFRAME, &err) != S_OK)
		{
			pthread_mutex_unlock(&m_mutex);
			printf("Could not open serial port (%s)\n", ERR_TO_STR(err));
			goto bail;
		}
		
		// wait for a deck to be connected
		m_waitingForDeckConnected = true;
		pthread_cond_wait(&m_condition, &m_mutex);
		m_waitingForDeckConnected = false;
	pthread_mutex_unlock(&m_mutex);
	
	// set deck preroll and offset
	m_deckControl->SetPreroll(5);
	m_deckControl->SetExportOffset(0);
	
	result = true;
	
bail:
	return result;
}

// Close IDeckLinkDeckControl
void	ETTHelper::cleanupDeckControl()
{
	m_deckControl->Close(false);
	m_deckControl->SetCallback(NULL);
}

// Stop video output
void	ETTHelper::cleanupDeckLinkOutput()
{
	m_deckLinkOutput->StopScheduledPlayback(0, NULL, 0);
	m_deckLinkOutput->DisableVideoOutput();
	m_deckLinkOutput->SetScheduledFrameCompletionCallback(NULL);
	releaseFrames();
}

// Start an export to tape operation
void	ETTHelper::doExport()
{
	BMDDeckControlExportModeOpsFlags exportModeOps = bmdDeckControlExportModeInsertVideo;
	BMDDeckControlError err;
	
	// setup DeckLink Output interface
	if (! setupDeckLinkOutput())
		goto bail;

	// setup DeckControl interface
	if (! setupDeckControl())
		goto bail;
	
	// preroll a few frames
	if (! scheduleNextFrame(true))
		goto bail;
	
	pthread_mutex_unlock(&m_mutex);
		// start export
		if (m_deckControl->StartExport(START_TC, STOP_TC, exportModeOps, &err) == S_OK)
		{
			// wait for export to finish or error to occur
			m_waitingForExportEnd = true;
			pthread_cond_wait(&m_condition, &m_mutex);
			m_waitingForExportEnd = false;
		}
		else
			printf("Could not start export (%s)\n", ERR_TO_STR(err));
	pthread_mutex_unlock(&m_mutex);
		
bail:
	cleanupDeckControl();
	cleanupDeckLinkOutput();
}



#pragma mark IDeckLinkDeckControlStatusCallback methods
/*
 * IDeckLinkDeckControlStatusCallback methods
 */
HRESULT ETTHelper::DeckControlEventReceived (/* in */ BMDDeckControlEvent event, /* in */ BMDDeckControlError error)
{
	printf("\n === '%s' event (error: %s)\n", EVT_TO_STR(event), ERR_TO_STR(error));
	
	switch (event){
		case bmdDeckControlPrepareForExportEvent:
			// We receive this event a few frames before the inpoint. 
			// At this point, we must call IDeckLinkOutput::StartScheduledPlayback()
			printf("Starting playback\n");
			if (m_deckLinkOutput->StartScheduledPlayback(0, m_timeScale, 1.0) == S_OK)
				m_exportStarted = true;
			else
				printf("Error starting playback\n");
			break;
		case bmdDeckControlExportCompleteEvent:
			// We receive this event a few frames after the out-point.
			// It is now safe to unblock the main thread, close the 
			// connection to the deck and release the IDeckLinkDeckControl 
			// and IDeckLinkOutput interfaces
	
			// fallthrough
			
		case bmdDeckControlAbortedEvent:
			m_exportStarted = false;
			
			// unblock main thread
			pthread_mutex_lock(&m_mutex);
				if (m_waitingForExportEnd)
					pthread_cond_signal(&m_condition);
			pthread_mutex_unlock(&m_mutex);
			break;
	}
	
	return S_OK;
}

HRESULT ETTHelper::DeckControlStatusChanged (/* in */ BMDDeckControlStatusFlags flags, /* in */ uint32_t mask)
{
	
	printf("\n ===  State change callback:\n");
	printf("New status: %s - %s - %s - %s\n", FLAGS_TO_STRS(flags));
	
	// unblock the main thread if waiting for a deck to be connected
	pthread_mutex_lock(&m_mutex);
		if ((m_waitingForDeckConnected) && (mask & bmdDeckControlStatusDeckConnected) && (flags & bmdDeckControlStatusDeckConnected))
			pthread_cond_signal(&m_condition);
	pthread_mutex_unlock(&m_mutex);
	
	return S_OK;
}



#pragma mark IDeckLinkVideoOutputCallback methods
/*
 * IDeckLinkVideoOutputCallback methods
 */
HRESULT	ETTHelper::ScheduledFrameCompleted (IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult result)
{
	if (m_exportStarted)
		// Schedule a new frame
		scheduleNextFrame(false);
	
	return S_OK;
}

HRESULT	ETTHelper::QueryInterface (REFIID iid, LPVOID *ppv)
{
	*ppv = NULL;
	return E_NOINTERFACE;
}

ULONG	ETTHelper::AddRef ()
{
	return ++m_refCount;
}

ULONG	ETTHelper::Release ()
{
	ULONG		newRefValue;
	
	newRefValue = --m_refCount;
	if (newRefValue == 0)
	{
		delete this;
		return 0;
	}
	
	return newRefValue;
}

#pragma mark CTOR DTOR
/*
 * CTOR DTOR
 */
ETTHelper::ETTHelper(IDeckLink *deckLink)
	: m_refCount(1), m_deckLink(deckLink), m_deckControl(NULL), m_deckLinkOutput(NULL)
	, m_waitingForDeckConnected(false), m_waitingForExportEnd(false), m_exportStarted(false)
	, m_videoFrames(NULL), m_nextFrameIndex(0), m_totalFrameScheduled(0)
	, m_width(-1), m_height(-1), m_timeScale(0), m_frameDuration(0)
{
	// create mutex and condition variable
	m_deckLink->AddRef();
	pthread_mutex_init(&m_mutex, NULL);
	pthread_cond_init(&m_condition, NULL);
}	

ETTHelper::~ETTHelper()
{
	// release IDeckLinkOutput and IDeckLinkDeckControl if required
	if (m_deckControl)
	{
		m_deckControl->Release();
		m_deckControl = NULL;
	}
	if (m_deckLinkOutput)
	{
		m_deckLinkOutput->Release();
		m_deckLinkOutput = NULL;
	}
	
	// destroy mutex and condition variable
	pthread_mutex_destroy(&m_mutex);
	pthread_cond_destroy(&m_condition);
	
	if (m_deckLink)
	{
		m_deckLink->Release();
		m_deckLink = NULL;
	}
}
								 
// Call init() before any other method. if init() fails, destroy the object
bool	ETTHelper::init()
{
	bool result = false;
	
	// get IDeckLinkOutput and IDeckLinkDeckControl
	if (m_deckLink->QueryInterface(IID_IDeckLinkOutput, (void **)&m_deckLinkOutput) != S_OK)
	{
		printf("Could not get DeckLink Output interface\n");
		m_deckLinkOutput = NULL;
		goto bail;
	}	
	
	if (m_deckLink->QueryInterface(IID_IDeckLinkDeckControl, (void **)&m_deckControl) != S_OK)
	{
		printf("Could not obtain the DeckControl interface\n");
		m_deckControl = NULL;
		goto bail;
	}
	
	result = true;
	
bail:
	return result;
	
}
