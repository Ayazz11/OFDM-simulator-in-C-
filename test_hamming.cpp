#include "SystemConfig.hpp"
#include "BitSource.hpp"
#include "SecdedEncoder.hpp"
#include "QamModulator.hpp"
#include "OfdmModulator.hpp"
#include "Channel.hpp"
#include "SecdedDecoder.hpp"
#include <iostream>
#include <complex>
using namespace std;
using cf32 = complex<float>;
#include <vector>
#include <cstdint>
#include <algorithm>

// This function wil; be used at last, after having the recieved bits from ofdm and qam demodulation we need to decode the bits.
//It takes 16 bit codewords and return the corresponding infoBits transmitted
vector<uint8_t> decodeHammingChunks(const std::vector<uint8_t>& codedBits)
{
    SecdedDecoder decoder;
    std::vector<uint8_t> recoveredInfoBits;

    // Each coded block is 16 bits -> 11 info bits
    for (std::size_t i = 0; i < codedBits.size(); i += 16)
    {
        // take one 16-bit codeword
        std::vector<uint8_t> codeword;
        codeword.reserve(16);

        for (std::size_t j = i; j < std::min<std::size_t>(i + 16, codedBits.size()); ++j)
        {
            codeword.push_back(codedBits[j]);
        }

        // If the final block is incomplete, skip it or pad before decoding
        if (codeword.size() != 16)
        {
            break;
        }

        SecdedDecodeResult r = decoder.decode(codeword);

        // append the 11 recovered info bits
        recoveredInfoBits.insert(
            recoveredInfoBits.end(),
            r.infoBits.begin(),
            r.infoBits.end()
        );
    }
    return recoveredInfoBits, r.status;
}

int main(){
    SystemConfig config;
    size_t bps = config.bitsPerSymbol; 
    size_t N = 208000; // number of bits transmitted
    BitSource b1(42);
    HammingEncoder h1;
    QamModulator qamMod(bps);
    OfdmModulator ofdmMod(config);
    Channel c(100);
    //Actual information bits at transmitted, N bits : 
    vector <uint8_t> infoBits = b1.generate(N);
    cout<<"Actual Transmitted Bits size :"<<infoBits.size()<<endl;
    vector <uint8_t> tx_codedBits;
    vector <Complex> tx_qamAll;
    //Encoded the transmit bits in chunks of 11 bits :
    for(size_t i=0; i<infoBits.size();i+=11){
        //initialzing the chunk with fixed size of 11 and values 0's because the last chunk might be of lesser size.
        vector <uint8_t> chunk(11,0);
        for(size_t j=0; j<(11) && (i+j<N); j++){
            chunk[j] = infoBits[i + j];
        }
        auto coded_chunk = h1.encode(chunk);
        tx_codedBits.insert(tx_codedBits.end(), coded_chunk.begin(), coded_chunk.end());
        auto qamSymbols_of_chunk = qamMod.modulate(coded_chunk);
        tx_qamAll.insert(tx_qamAll.end(), qamSymbols_of_chunk.begin(), qamSymbols_of_chunk.end());
    }
cout<<"Number of coded bits : "<<tx_codedBits.size()<<endl;
cout<<"Number of qam symbols : "<<tx_qamAll.size()<<endl;
//Perform OFDM Modulation
vector <uint8_t> recievedBitsAll;
vector <uint8_t> recoveredInfoAll;
vector <Complex> tx_signal_All;
vector <Complex> rx_signal_All;
vector <Complex> rx_qam_All;
vector <Complex> Valid_rx_qam_All;
//ofdm input needs to be of fixed size i.e 52, so we have to break tx_qamAll into chunks of 52.//QAM input to ofdm Modulator at a time
for (size_t i = 0; i < tx_qamAll.size(); i += 52) {
    vector<Complex> ofdmInput(52, Complex(0.0f, 0.0f)); 
    size_t count = min<size_t>(52, tx_qamAll.size() - i);
    copy(tx_qamAll.begin() + i, tx_qamAll.begin() + i + count, ofdmInput.begin());
    //ofdmInput is now fed to ofdm modulator
    auto tx_signal = ofdmMod.modulate(ofdmInput);
    tx_signal_All.insert(tx_signal_All.end(), tx_signal.begin(), tx_signal.end());
    auto rx_signal=c.addAwgn(tx_signal,5,bps);
    rx_signal_All.insert(rx_signal_All.end(), rx_signal.begin(), rx_signal.end());
    auto rxQam = ofdmMod.demodulate(rx_signal);
    rx_qam_All.insert(rx_qam_All.end(), rxQam.begin(), rxQam.end());
    // keep only the valid qam symbols : we remove the extra qam symbols added (0.0f,0.0f) in the last ofdmInput block.
    vector<Complex> validRx(rxQam.begin(), rxQam.begin() + count);
    Valid_rx_qam_All.insert(Valid_rx_qam_All.end(), validRx.begin(), validRx.end());
    auto recievedBits = qamMod.demodulate(validRx);
    recievedBitsAll.insert(recievedBitsAll.end(), recievedBits.begin(), recievedBits.end());
    // then decode 16-bit codewords to 11 bits info
    auto recoveredInfoBits = decodeHammingChunks(recievedBits)[0];
    recoveredInfoAll.insert(recoveredInfoAll.end(), recoveredInfoBits.begin(), recoveredInfoBits.end());
}
// we remove the extra bits added (0) in the last chunk of 11 bits.
std::vector<uint8_t> validRecoveredInfo;
validRecoveredInfo.reserve(infoBits.size());
for (size_t i = 0; i < recoveredInfoAll.size() && i < infoBits.size(); ++i)
{
    validRecoveredInfo.push_back(recoveredInfoAll[i]);
}
int error_count=0;
for(size_t i=0;i<infoBits.size();i++){
    if(validRecoveredInfo[i]!=infoBits[i]){
        error_count++;
    }
}
cout<<"tx signal size (Ofdm modulated) : "<<tx_signal_All.size()<<endl;
cout<<"rx signal size  : "<<rx_signal_All.size()<<endl;
cout<<"rx Qam symbols size  : "<<rx_qam_All.size()<<endl;
cout<<"Valid rx Qam symbols size  : "<<Valid_rx_qam_All.size()<<endl;
cout<<"Recieved bits size  : "<<recievedBitsAll.size()<<endl;
cout<<"Recovered bits size  : "<<recoveredInfoAll.size()<<endl;
cout<<"Valid Recovered bits size  : "<<validRecoveredInfo.size()<<endl;
cout<<"Error count : "<<error_count<<endl; 
cout<<"BER :"<<double( double(error_count) / double(N) )<<endl;
//Without any coding/decoding Block  : 
vector<uint8_t> uncodedInfoBits = infoBits;
vector<uint8_t> uncodedReceivedBitsAll;
vector<Complex> uncodedTxSignalAll;
vector<Complex> uncodedRxSignalAll;
vector<Complex> uncodedRxQamAll;
vector<Complex> uncodedValidRxQamAll;

if (uncodedInfoBits.size() % bps != 0) {
    size_t pad = bps - (uncodedInfoBits.size() % bps);
    uncodedInfoBits.insert(uncodedInfoBits.end(), pad, 0);
}

auto uncodedQamModulated = qamMod.modulate(uncodedInfoBits);
for (size_t i = 0; i < uncodedQamModulated.size(); i += 52) {
    vector<Complex> uncodedOfdmInput(52, Complex(0.0f, 0.0f));

    size_t count = min<size_t>(52, uncodedQamModulated.size() - i);
    copy(uncodedQamModulated.begin() + i, uncodedQamModulated.begin() + i + count, uncodedOfdmInput.begin());

    auto uncodedTxSignal = ofdmMod.modulate(uncodedOfdmInput);
    uncodedTxSignalAll.insert(uncodedTxSignalAll.end(), uncodedTxSignal.begin(), uncodedTxSignal.end());

    auto uncodedRxSignal = c.addAwgn(uncodedTxSignal, 5, bps);
    uncodedRxSignalAll.insert(uncodedRxSignalAll.end(), uncodedRxSignal.begin(), uncodedRxSignal.end());

    auto uncodedRxQam = ofdmMod.demodulate(uncodedRxSignal);
    uncodedRxQamAll.insert(uncodedRxQamAll.end(), uncodedRxQam.begin(), uncodedRxQam.end());

    vector<Complex> uncodedValidRx(uncodedRxQam.begin(), uncodedRxQam.begin() + count);
    uncodedValidRxQamAll.insert(uncodedValidRxQamAll.end(), uncodedValidRx.begin(), uncodedValidRx.end());

    auto uncodedReceivedBits = qamMod.demodulate(uncodedValidRx);
    uncodedReceivedBitsAll.insert(uncodedReceivedBitsAll.end(), uncodedReceivedBits.begin(), uncodedReceivedBits.end());
}

size_t uncodedErrorCount = 0;
for(size_t i = 0; i < uncodedInfoBits.size(); ++i){
    if(uncodedReceivedBitsAll[i] != uncodedInfoBits[i]){
        uncodedErrorCount++;
    }
}
cout<<endl;
cout << "============================= " <<endl;
cout<< " Without coding and decoding Block :" <<endl;
cout << "Uncoded tx signal size (Ofdm modulated) : " << uncodedTxSignalAll.size() << endl;
cout << "Uncoded rx signal size  : " << uncodedRxSignalAll.size() << endl;
cout << "Uncoded rx Qam symbols size  : " << uncodedRxQamAll.size() << endl;
cout << "Uncoded valid rx Qam symbols size  : " << uncodedValidRxQamAll.size() << endl;
cout << "Uncoded received bits size  : " << uncodedReceivedBitsAll.size() << endl;
cout << "Uncoded error count : " << uncodedErrorCount << endl;

}