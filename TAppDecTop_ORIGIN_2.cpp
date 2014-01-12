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
#define BUFSIZE 4000
SOCKET input_sock,hClnt_Sock;
WSADATA wsaData1;
SOCKADDR_IN servAddr1,clntAddr1;
int mod;
int len,clntAddr1Size;
//char buf[BUFSIZE];

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
DWORD WINAPI isThreadFunc(LPVOID lpParm)
{
	char buf[BUFSIZE];
	stringstream* Recv=(stringstream*)lpParm;
	while( (len=recv(hClnt_Sock, buf, BUFSIZE, 0)) != 0 )//정상적으로 recv 하지만 write를 안함
	{
			Recv->write(buf,len);
			printf("recv_Size: %d\n",len);
			//cout<<Recv->width()<<endl;
		}
	return 0;
}

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
	//========================================================
	//
	//prepare the thread.
	//========================================================

	switch(mod)
	{
	case 3://udp
		if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0)
			ErrorMSG("WSAstartup() error!");
		input_sock=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
		if(input_sock==INVALID_SOCKET)//소켓 에러 확인
		{
			ErrorMSG("UDP_socket() error");
		}
		memset(&servAddr1,0,sizeof(servAddr1));
		servAddr1.sin_family=AF_INET;
		servAddr1.sin_addr.s_addr=inet_addr(str1.c_str());//inet_addr function : change string->ip_addr_number
		servAddr1.sin_port=htons(atoi(str2.c_str()));
		//recv(in_tcp_Sock,buf,BUFSIZE,0);
		if(bind(input_sock,(SOCKADDR*)&servAddr1,sizeof(servAddr1))==SOCKET_ERROR)//bind to server
		{
			ErrorMSG("bind() error!");
		}
		if(listen(input_sock,5)==SOCKET_ERROR)
			ErrorMSG("listen() err");
		clntAddr1Size=sizeof(clntAddr1);

		hClnt_Sock=accept(input_sock,(struct sockaddr*)&clntAddr1,&clntAddr1Size);
		if(hClnt_Sock==INVALID_SOCKET)
			ErrorMSG("accept() error");
		break;
	case 2://tcp
		if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0)
			ErrorMSG("WSAstartup() error!");
		input_sock=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(input_sock==INVALID_SOCKET)//소켓 에러 확인
		{
			ErrorMSG("tcp_socket() error");
		}
		memset(&servAddr1,0,sizeof(servAddr1));
		servAddr1.sin_family=AF_INET;
		servAddr1.sin_addr.s_addr=htonl(INADDR_ANY);//inet_addr function : change string->ip_addr_number
		servAddr1.sin_port=htons(atoi(str2.c_str()));
		//recv(in_tcp_Sock,buf,BUFSIZE,0);
		if(bind(input_sock,(SOCKADDR*)&servAddr1,sizeof(servAddr1))==SOCKET_ERROR)//bind to server
		{
			ErrorMSG("bind() error!");
		}
		if(listen(input_sock,5)==SOCKET_ERROR)
			ErrorMSG("listen() err");
		clntAddr1Size=sizeof(clntAddr1);
		hClnt_Sock=accept(input_sock,(struct sockaddr*)&clntAddr1,&clntAddr1Size);
		if(hClnt_Sock==INVALID_SOCKET)
			ErrorMSG("accept() error");
		break;
	}

	//========================================================
	Int                 poc;
	TComList<TComPic*>* pcListPic = NULL;
	//DWORD dwThID=0;
	switch(mod)
	{
	case 3:
		{

		stringstream bitstreamFile;
		CreateThread(NULL,0,isThreadFunc,&bitstreamFile,0,NULL);
		Sleep(1000);
		InputByteStream bytestream(bitstreamFile);

		// create & initialize internal classes
		xCreateDecLib();
		xInitDecLib  ();
		m_iPOCLastDisplay += m_iSkipFrame;      // set the last displayed POC correctly for skip forward.

		// main decoder loop
		Bool recon_opened = false; // reconstruction file not yet opened. (must be performed after SPS is seen)
		while (!!bitstreamFile)
		{
			/* location serves to work around a design fault in the decoder, whereby
			* the process of reading a new slice that is the first slice of a new frame
			* requires the TDecTop::decode() method to be called again with the same
			* nal unit. */
			streampos location = bitstreamFile.tellg();
			AnnexBStats stats = AnnexBStats();
			Bool bPreviousPictureDecoded = false;

			vector<uint8_t> nalUnit;
			InputNALUnit nalu;
			byteStreamNALUnit(bytestream, nalUnit, stats);

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
				read(nalu, nalUnit);
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
					if (bNewPicture)
					{
						bitstreamFile.clear();
						/* location points to the current nalunit payload[1] due to the
						* need for the annexB parser to read three extra bytes.
						* [1] except for the first NAL unit in the file
						*     (but bNewPicture doesn't happen then) */
						bitstreamFile.seekg(location-streamoff(3));
						bytestream.reset();
					}
					bPreviousPictureDecoded = true; 
				}
			}
			if (bNewPicture || !bitstreamFile)
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
		break;
		}
	case 2:{
		stringstream bitstreamFile;
		//HANDLE h;
		CreateThread(NULL,0,isThreadFunc,&bitstreamFile,0,NULL);
		//WaitForSingleObject(h,INFINITE);
		Sleep(1000);
		InputByteStream bytestream(bitstreamFile);

		// create & initialize internal classes
		xCreateDecLib();
		xInitDecLib  ();
		m_iPOCLastDisplay += m_iSkipFrame;      // set the last displayed POC correctly for skip forward.

		// main decoder loop
		Bool recon_opened = false; // reconstruction file not yet opened. (must be performed after SPS is seen)
		while (!!bitstreamFile)
		{
			/* location serves to work around a design fault in the decoder, whereby
			* the process of reading a new slice that is the first slice of a new frame
			* requires the TDecTop::decode() method to be called again with the same
			* nal unit. */
			streampos location = bitstreamFile.tellg();
			AnnexBStats stats = AnnexBStats();
			Bool bPreviousPictureDecoded = false;

			vector<uint8_t> nalUnit;
			InputNALUnit nalu;
			byteStreamNALUnit(bytestream, nalUnit, stats);

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
				read(nalu, nalUnit);
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
					if (bNewPicture)
					{
						bitstreamFile.clear();
						/* location points to the current nalunit payload[1] due to the
						* need for the annexB parser to read three extra bytes.
						* [1] except for the first NAL unit in the file
						*     (but bNewPicture doesn't happen then) */
						bitstreamFile.seekg(location-streamoff(3));
						bytestream.reset();
					}
					bPreviousPictureDecoded = true; 
				}
			}
			if (bNewPicture || !bitstreamFile)
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
		break;
		}
	case 1:
		ifstream bitstreamFile(m_pchBitstreamFile, ifstream::in | ifstream::binary);
	

		if (!bitstreamFile)
		{
			fprintf(stderr, "\nfailed to open bitstream file `%s' for reading\n", m_pchBitstreamFile);
			exit(EXIT_FAILURE);
		}
		InputByteStream bytestream(bitstreamFile);

		// create & initialize internal classes
		xCreateDecLib();
		xInitDecLib  ();
		m_iPOCLastDisplay += m_iSkipFrame;      // set the last displayed POC correctly for skip forward.

		// main decoder loop
		Bool recon_opened = false; // reconstruction file not yet opened. (must be performed after SPS is seen)
		while (!!bitstreamFile)
		{
			/* location serves to work around a design fault in the decoder, whereby
			* the process of reading a new slice that is the first slice of a new frame
			* requires the TDecTop::decode() method to be called again with the same
			* nal unit. */
			streampos location = bitstreamFile.tellg();
			AnnexBStats stats = AnnexBStats();
			Bool bPreviousPictureDecoded = false;

			vector<uint8_t> nalUnit;
			nalUnit.size();
			InputNALUnit nalu;
			byteStreamNALUnit(bytestream, nalUnit, stats);

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
				read(nalu, nalUnit);
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
					if (bNewPicture)
					{
						bitstreamFile.clear();
						/* location points to the current nalunit payload[1] due to the
						* need for the annexB parser to read three extra bytes.
						* [1] except for the first NAL unit in the file
						*     (but bNewPicture doesn't happen then) */
						bitstreamFile.seekg(location-streamoff(3));
						bytestream.reset();
					}
					bPreviousPictureDecoded = true; 
				}
			}
			if (bNewPicture || !bitstreamFile)
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
		//	}
	}//end switch case
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
