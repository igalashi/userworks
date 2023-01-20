#include <vector>

#ifdef __ROOTCLING__ 
#pragma link C++ nestedclasses;
#pragma link C++ nestedtypedefs;

#pragma link C++ class std::vector<std::vector<bool>>+;
#pragma link C++ class std::vector<std::vector<bool>>::*;

#pragma link C++ class std::vector<std::vector<char>>+;
#pragma link C++ class std::vector<std::vector<char>>::*;
#pragma link C++ class std::vector<std::vector<short>>+;
#pragma link C++ class std::vector<std::vector<short>>::*;
#pragma link C++ class std::vector<std::vector<int>>+;
#pragma link C++ class std::vector<std::vector<int>>::*;
#pragma link C++ class std::vector<std::vector<long long>>+;
#pragma link C++ class std::vector<std::vector<long long>>::*;

#pragma link C++ class std::vector<std::vector<unsigned char>>+;
#pragma link C++ class std::vector<std::vector<unsigned char>>::*;
#pragma link C++ class std::vector<std::vector<unsigned short>>+;
#pragma link C++ class std::vector<std::vector<unsigned short>>::*;
#pragma link C++ class std::vector<std::vector<unsigned int>>+;
#pragma link C++ class std::vector<std::vector<unsigned int>>::*;
#pragma link C++ class std::vector<std::vector<unsigned long long>>+;
#pragma link C++ class std::vector<std::vector<unsigned long long>>::*;

#pragma link C++ class std::vector<std::vector<float>>+;
#pragma link C++ class std::vector<std::vector<float>>::*;
#pragma link C++ class std::vector<std::vector<double>>+;
#pragma link C++ class std::vector<std::vector<double>>::*;


#pragma link C++ class std::vector<std::vector<Bool_t>>+;
#pragma link C++ class std::vector<std::vector<Bool_t>>::*;

#pragma link C++ class std::vector<std::vector<Char_t>>+;
#pragma link C++ class std::vector<std::vector<Char_t>>::*;
#pragma link C++ class std::vector<std::vector<Short_t>>+;
#pragma link C++ class std::vector<std::vector<Short_t>>::*;
#pragma link C++ class std::vector<std::vector<Int_t>>+;
#pragma link C++ class std::vector<std::vector<Int_t>>::*;
#pragma link C++ class std::vector<std::vector<Long64_t>>+;
#pragma link C++ class std::vector<std::vector<Long64_t>>::*;

#pragma link C++ class std::vector<std::vector<UChar_t>>+;
#pragma link C++ class std::vector<std::vector<UChar_t>>::*;
#pragma link C++ class std::vector<std::vector<UShort_t>>+;
#pragma link C++ class std::vector<std::vector<UShort_t>>::*;
#pragma link C++ class std::vector<std::vector<UInt_t>>+;
#pragma link C++ class std::vector<std::vector<UInt_t>>::*;
#pragma link C++ class std::vector<std::vector<ULong64_t>>+;
#pragma link C++ class std::vector<std::vector<ULong64_t>>::*;

#pragma link C++ class std::vector<std::vector<Float_t>>+;
#pragma link C++ class std::vector<std::vector<Float_t>>::*;
#pragma link C++ class std::vector<std::vector<Double_t>>+;
#pragma link C++ class std::vector<std::vector<Double_t>>::*;

#ifdef G__VECTOR_HAS_CLASS_ITERATOR

#pragma link C++ operators std::vector<std::vector<bool>>::iterator;
#pragma link C++ operators std::vector<std::vector<bool>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<bool>>::reverse_iterator;

#pragma link C++ operators std::vector<std::vector<char>>::iterator;
#pragma link C++ operators std::vector<std::vector<char>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<char>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<short>>::iterator;
#pragma link C++ operators std::vector<std::vector<short>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<short>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<int>>::iterator;
#pragma link C++ operators std::vector<std::vector<int>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<int>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<long long>>::iterator;
#pragma link C++ operators std::vector<std::vector<long long>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<long long>>::reverse_iterator;

#pragma link C++ operators std::vector<std::vector<unsigned char>>::iterator;
#pragma link C++ operators std::vector<std::vector<unsigned char>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<unsigned char>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<unsigned short>>::iterator;
#pragma link C++ operators std::vector<std::vector<unsigned short>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<unsigned short>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<unsigned int>>::iterator;
#pragma link C++ operators std::vector<std::vector<unsigned int>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<unsigned int>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<unsigned long long>>::iterator;
#pragma link C++ operators std::vector<std::vector<unsigned long long>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<unsigned long long>>::reverse_iterator;

#pragma link C++ operators std::vector<std::vector<float>>::iterator;
#pragma link C++ operators std::vector<std::vector<float>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<float>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<double>>::iterator;
#pragma link C++ operators std::vector<std::vector<double>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<double>>::reverse_iterator;

#pragma link C++ operators std::vector<std::vector<Bool_t>>::iterator;
#pragma link C++ operators std::vector<std::vector<Bool_t>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<Bool_t>>::reverse_iterator;

#pragma link C++ operators std::vector<std::vector<Char_t>>::iterator;
#pragma link C++ operators std::vector<std::vector<Char_t>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<Char_t>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<Short_t>>::iterator;
#pragma link C++ operators std::vector<std::vector<Short_t>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<Short_t>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<Int_t>>::iterator;
#pragma link C++ operators std::vector<std::vector<Int_t>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<Int_t>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<Long64_t>>::iterator;
#pragma link C++ operators std::vector<std::vector<Long64_t>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<Long64_t>>::reverse_iterator;

#pragma link C++ operators std::vector<std::vector<UChar_t>>::iterator;
#pragma link C++ operators std::vector<std::vector<UChar_t>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<UChar_t>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<UShort_t>>::iterator;
#pragma link C++ operators std::vector<std::vector<UShort_t>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<UShort_t>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<UInt_t>>::iterator;
#pragma link C++ operators std::vector<std::vector<UInt_t>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<UInt_t>>::reverse_iterator;
#pragma link C++ operators std::vector<std::vector<ULong64_t>>::iterator;
#pragma link C++ operators std::vector<std::vector<ULong64_t>>::const_iterator;
#pragma link C++ operators std::vector<std::vector<ULong64_t>>::reverse_iterator;




#endif
#endif
