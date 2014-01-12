/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.  
*
* Copyright (c) 2010-2013, ITU/ISO/IEC
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

/** \file     TAppDecTop.cpp
\brief    Decoder application class
*/
#include <list>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include "TAppDecTop.h"
#include "TLibDecoder/AnnexBread.h"
#include "TLibDecoder/NALread.h"
//#pragma comment (lib,"ws2_32.lib")
//#include <WinSock2.h>

//! \ingroup TAppDecoder
//! \{
// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================
SOCKET input_sock=-1;
SOCKET hClnt_Sock;
WSADATA wsaData1;
SOCKADDR_IN servAddr1,clntAddr1;
int mod;

int len,clntAddr1Size;

//=========================================
// thread function
//=========================================
void ErrorMSG(char *message);
TAppDecTop::TAppDecTop()
	: m_iPOCLastDisplay(-MAX_INT)
{
	::memset (m_abDecFlag, 0, sizeof (m_abDecFlag));
}
Void TAppDecTop::create()
{
}

Void TAppDecTop::destroy()
{
	if (m_pchBitstreamFile)
	{
		free (m_pchBitstreamFile);
		m_pchBitstreamFile = NULL;
	}
	if (m_pchReconFile)
	{
		free (m_pchReconFile);
		m_pchReconFile = NULL;
	}
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
- create internal class
- initialize internal class
- until the end of the bitstream, call decoding function in TDecTop class
- delete allocated buffers
- destroy internal class
.
*/

Void TAppDecTop::decode()
{
	//=======================================
	//input param
	//형식에 맞추지 않으면 abort err라 난다!!
	//=======================================
	//=======================================
	//output param
	//=======================================
	string tmpAddr=m_pchBitstreamFile;//6~18까지
	string str1="";
	string str2="";
	int i=6;
	//cout<<tmpAddr[1]<<' '<<&tmpAddr[1]<<endl;
	if(strstr(m_pchBitstreamFile,"udp")!=0||strstr(m_pchBitstreamFile,"tcp")!=0){
		while(tmpAddr.at(i)!=':')//127.0.0.1  ip 주소
		{
			str1.append(1,tmpAddr.at(i));
			i++;
		}
		i++;
		while(i!=tmpAddr.length())//reversed port number 
		{
			str2.append(1,tmpAddr.at(i));
			i++;

		}
	}
	//======================================================
	// test lab
	//======================================================
	if(strstr(m_pchBitstreamFile,"tcp://")!=0)mod=2;
	else if(strstr(m_pchBitstreamFile,"udp://")!=0)mod=3;
	else mod=1;
	bool eof; //YMK
	
	//======================================================================
	//2013.01.03 세미나 준비 자료
	//decode code 분석
	//======================================================================
	Int                 poc;
	TComList<TComPic*>* pcListPic = NULL;
	ifstream bitstreamFile(m_pchBitstreamFile, ifstream::in|ifstream::binary);
	Socketstream bitstreamSock(str2.c_str());//CSH char* -> string 
	bitstreamSock.initialize(mod); //CSH MOD를 파라미터로 넘겨줌. TCP,UCP 경우를 SWITCH-CASE 문으로 초기화.
	InputByteStream bytestream(bitstreamFile,bitstreamSock,mod);//YMK end...

	if (eof=
		(mod==1)?bitstreamFile.eof():
		((mod==2||mod==3)?(bitstreamSock.eof()) : true)) //CSH
	{
		fprintf(stderr, "\nfailed to open bitstream file `%s' for reading\n", m_pchBitstreamFile);
		exit(EXIT_FAILURE);
	}
		// create & initialize internal classes
		xCreateDecLib();
		xInitDecLib  ();
		m_iPOCLastDisplay += m_iSkipFrame;      // set the last displayed POC correctly for skip forward.

		// main decoder loop
		Bool recon_opened = false; // reconstruction file not yet opened. (must be performed after SPS is seen)
		Bool bHaveNAL=false; //YNK
		vector<uint8_t> nalUnit;
		AnnexBStats stats = AnnexBStats();
		while (!eof)
		{
			/* location serves to work around a design fault in the decoder, whereby
			* the process of reading a new slice that is the first slice of a new frame
			* requires the TDecTop::decode() method to be called again with the same
			* nal unit. */
			//semi_size=bitstreamFile.tellg();//의도: 한번 읽어 오는 스트림의 크기
			//total_size+=semi_size;
			//printf("count: %d , size: %d , total: %d\n",semi_cnt++,semi_size,total_size);
			//결과: 한번에 읽어 오지 않고 hdd에 저장된 상태에서도 조금씩 떼서 읽는다.
			// (YMK) streampos location = bitstreamFile.tellg();
			/*what is tellg()? return the current position in stream.*/
			// (YMK) AnnexBStats stats = AnnexBStats();
			Bool bPreviousPictureDecoded = false;

			// (YMK) vector<uint8_t> nalUnit;
			//unsigned int 1byte type의 벡터 클래스 생성
			//return size of nalUnit
			InputNALUnit nalu;
			//nulUnit에 대한 간이 rapper class
			// (YMK) begin
			if(!bHaveNAL)
			{
				nalUnit.resize(0);
				memset(&stats,0,sizeof(AnnexBStats));
				byteStreamNALUnit(bytestream, nalUnit, stats);
			}
			//(YMK) end.....
			//stats에 bytestream statisticd을 축적 하는 동안 bytestream에서 nalUnit을 추출 함

			// call actual decoding function
			Bool bNewPicture = false;
			if (nalUnit.empty())
			{
				/* this can happen if the following occur:
				*  - empty input file
				*  - two back-to-back start_code_prefixes
				*  - start_code_prefix immediately followed by EOF
				*/
				fprintf(stderr, "Warning: Attempt to decode an empty NAL unit\n");
			}
			else
			{
				read(nalu, nalUnit,bHaveNAL);
				bHaveNAL=false;
				if( (m_iMaxTemporalLayer >= 0 && nalu.m_temporalId > m_iMaxTemporalLayer) || !isNaluWithinTargetDecLayerIdSet(&nalu)  )
				{
					if(bPreviousPictureDecoded)
					{
						bNewPicture = true;
						bPreviousPictureDecoded = false;
					}
					else
					{
						bNewPicture = false;
					}
				}
				else
				{
					
					bNewPicture = m_cTDecTop.decode(nalu, m_iSkipFrame, m_iPOCLastDisplay);
			//m-fifo에 complete byte가 저장 된다 . 
					if (bNewPicture)
					{
						//(YMK)bitstreamFile.clear();
						/* location points to the current nalunit payload[1] due to the
						* need for the annexB parser to read three extra bytes.
						* [1] except for the first NAL unit in the file
						*     (but bNewPicture doesn't happen then) */
						//주의 !!!
						//(YMK)bitstreamFile.seekg(location-streamoff(3));//Moves the read position in a stream.
						//(YMK)bytestream.reset();

						bHaveNAL=true;//(YMK)
					}
					bPreviousPictureDecoded = true; 
				}
			}
			if (bNewPicture ||(eof=(mod==1)?bitstreamFile.eof():((mod==2||mod==3)?(bitstreamSock.eof()) : true))) //CSH 
			{
				m_cTDecTop.executeLoopFilters(poc, pcListPic);
			}

			if( pcListPic )
			{
				if ( m_pchReconFile && !recon_opened )
				{
					if (!m_outputBitDepthY) { m_outputBitDepthY = g_bitDepthY; }
					if (!m_outputBitDepthC) { m_outputBitDepthC = g_bitDepthC; }
					//create socket
					m_cTVideoIOYuvReconFile.open( m_pchReconFile, true, m_outputBitDepthY, m_outputBitDepthC, g_bitDepthY, g_bitDepthC ); // write mode
					recon_opened = true;
				}
				if ( bNewPicture && 
					(   nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR
					|| nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP
					|| nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
					|| nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLANT
					|| nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA ) )
				{
					xFlushOutput( pcListPic );
				}
				// write reconstruction to file
				if(bNewPicture)
				{				
					xWriteOutput(pcListPic, nalu.m_temporalId );

				}
			}
		}


		xFlushOutput( pcListPic );
		// delete buffers
		m_cTDecTop.deletePicBuffer();

		// destroy internal classes
		xDestroyDecLib();
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TAppDecTop::xCreateDecLib()
{
	// create decoder class
	m_cTDecTop.create();
}

Void TAppDecTop::xDestroyDecLib()
{
	if ( m_pchReconFile )
	{
		m_cTVideoIOYuvReconFile. close();
	}

	// destroy decoder class
	m_cTDecTop.destroy();
}

Void TAppDecTop::xInitDecLib()
{
	// initialize decoder class
	m_cTDecTop.init();
	m_cTDecTop.setDecodedPictureHashSEIEnabled(m_decodedPictureHashSEIEnabled);
}

/** \param pcListPic list of pictures to be written to file
\todo            DYN_REF_FREE should be revised
*/
Void TAppDecTop::xWriteOutput(TComList<TComPic*>* pcListPic, UInt tId )
{
	TComList<TComPic*>::iterator iterPic   = pcListPic->begin();
	Int not_displayed = 0;

	while (iterPic != pcListPic->end())
	{
		TComPic* pcPic = *(iterPic);
		if(pcPic->getOutputMark() && pcPic->getPOC() > m_iPOCLastDisplay)
		{
			not_displayed++;
		}
		iterPic++;
	}
	iterPic   = pcListPic->begin();
	while (iterPic != pcListPic->end())
	{
		TComPic* pcPic = *(iterPic);

		if ( pcPic->getOutputMark() && (not_displayed >  pcPic->getNumReorderPics(tId) && pcPic->getPOC() > m_iPOCLastDisplay))
		{
			// write to file
			not_displayed--;
			if ( m_pchReconFile )//여기인듯...
			{
				const Window &conf = pcPic->getConformanceWindow();
				const Window &defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();
				m_cTVideoIOYuvReconFile.write(pcPic->getPicYuvRec(),
					conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
					conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
					conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),

					conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset() );
			}
			// update POC of display order
			m_iPOCLastDisplay = pcPic->getPOC();

			// erase non-referenced picture in the reference picture list after display
			if ( !pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true )
			{
#if !DYN_REF_FREE
				pcPic->setReconMark(false);

				// mark it should be extended later
				pcPic->getPicYuvRec()->setBorderExtension( false );

#else
				pcPic->destroy();
				pcListPic->erase( iterPic );
				iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
				continue;
#endif
			}
			pcPic->setOutputMark(false);
		}

		iterPic++;
	}
}

/** \param pcListPic list of pictures to be written to file
\todo            DYN_REF_FREE should be revised
*/
Void TAppDecTop::xFlushOutput( TComList<TComPic*>* pcListPic )
{
	if(!pcListPic)
	{
		return;
	} 
	TComList<TComPic*>::iterator iterPic   = pcListPic->begin();

	iterPic   = pcListPic->begin();

	while (iterPic != pcListPic->end())
	{
		TComPic* pcPic = *(iterPic);

		if ( pcPic->getOutputMark() )
		{
			// write to file
			if ( m_pchReconFile )
			{
				const Window &conf = pcPic->getConformanceWindow();
				const Window &defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();
				m_cTVideoIOYuvReconFile.write( pcPic->getPicYuvRec(),
					conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
					conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
					conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
					conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset() );
			}

			// update POC of display order
			m_iPOCLastDisplay = pcPic->getPOC();

			// erase non-referenced picture in the reference picture list after display
			if ( !pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true )
			{
#if !DYN_REF_FREE
				pcPic->setReconMark(false);

				// mark it should be extended later
				pcPic->getPicYuvRec()->setBorderExtension( false );

#else
				pcPic->destroy();
				pcListPic->erase( iterPic );
				iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
				continue;
#endif
			}
			pcPic->setOutputMark(false);
		}
#if !DYN_REF_FREE
		if(pcPic)
		{
			pcPic->destroy();
			delete pcPic;
			pcPic = NULL;
		}
#endif    
		iterPic++;
	}
	pcListPic->clear();
	m_iPOCLastDisplay = -MAX_INT;
}

/** \param nalu Input nalu to check whether its LayerId is within targetDecLayerIdSet
*/
Bool TAppDecTop::isNaluWithinTargetDecLayerIdSet( InputNALUnit* nalu )
{
	if ( m_targetDecLayerIdSet.size() == 0 ) // By default, the set is empty, meaning all LayerIds are allowed
	{
		return true;
	}
	for (std::vector<Int>::iterator it = m_targetDecLayerIdSet.begin(); it != m_targetDecLayerIdSet.end(); it++)
	{
		if ( nalu->m_reservedZero6Bits == (*it) )
		{
			return true;
		}
	}
	return false;
}


void ErrorMSG(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


//! \}
