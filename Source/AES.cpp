#include<fstream>
#include<cstring>
#include<random>
#include<exception>
#include"AES.hpp"
#include"OperationsGF256.hpp"

#define MAX_KEY_LENGTH_BYTES 32

/************************************* Default values for substitution boxes. This are the values showed in the standard ******************************************/


static const char a[4] 	  = {0x02, 0x03, 0x01, 0x01};				            // -For MixColumns.
static const char aInv[4] = {0x0E, 0x0B, 0x0D, 0x09};				            // -For InvMixColumns.

static const char Rcon[10][4] = {						                        // -Notice that the value of the left most char in polynomial form is 2^i.
	{0x01, 0x00, 0x00, 0x00},
  	{0x02, 0x00, 0x00, 0x00},
	{0x04, 0x00, 0x00, 0x00},
	{0x08, 0x00, 0x00, 0x00},
	{0x10, 0x00, 0x00, 0x00},
	{0x20, 0x00, 0x00, 0x00},
	{0x40, 0x00, 0x00, 0x00},
	{(char)0x80, 0x00, 0x00, 0x00},
	{0x1B, 0x00, 0x00, 0x00},
  	{0x36, 0x00, 0x00, 0x00}
};

static const uint8_t SBox[SBOX_SIZE] = {
	0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
	0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
	0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
	0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
	0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
	0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
	0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
	0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
	0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
	0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
	0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
	0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
	0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
	0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
	0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
	0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

static const uint8_t invSBox[SBOX_SIZE] = {
	0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
	0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
	0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
	0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
	0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
	0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
	0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
	0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
	0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
	0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
	0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
	0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
	0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
	0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
	0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
	0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D
};


/****************************************** Multiplication of two numbers of 256 bits to obtain a 512 bits number ************************************************/

union int_char {
    int32_t int_;                                                              // Useful when casting from a 32 bits integer to an array of four chars
    char    chars[4];
};
union uint64_uint32 {                                                           // -Useful for a casting from a 64 bits unsigned integer to an array of two 32 bits
    uint64_t uint64;                                                            //  integer
    unsigned uint32[2];
};
union _16uint32_64uchar {                                                       // -Representing 256 bits in an union of 8 unsigned int and 32 char
    unsigned uint32[16];
    uint8_t  chars[64];
};
struct Uint256 {
    union {                                                                     // -256 bits in a anonymous union of 8 unsigned int and 32 char
        uint8_t  chars[32];
        unsigned uint32[8];
    } NumberPlaces = {0,0,0,0,0,0,0,0};

    Uint256() {}
    Uint256(const char*const data) {
        for(int i = 0; i < 32; i++)
            NumberPlaces.chars[i] = (uint8_t)data[i];
    }

    unsigned operator [] (int i) const {
        if(i < 0 || i >= 8) i &= 7;                                             // -i&7 is equivalent to i%8
        return this->NumberPlaces.uint32[i];
    }
    void reWriteLeastSignificantBytes(const char*const array, size_t arraySize = 32) {    // -Rewrites the least significant bytes of the NumberPlaces union.
        if(arraySize > 32) arraySize &= 31;                                     // -Writing the array from left to right, those bytes would be the left ones.
        for(size_t i = 0; i < arraySize; i++)                                   // -arraySize % 32
            this->NumberPlaces.chars[i] = (uint8_t)array[i];                    // -The rest of the bytes (i >= arraySize) are left untouched
    }
};

static _16uint32_64uchar operator * (const Uint256& a, const Uint256& b) { // -Multiplying two integers of 256 bits each one
    int i, j;
    unsigned carriage = 0;
    uint64_uint32 buff = {0};
    _16uint32_64uchar result = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};               // -The result will need 512 bits (for arbitrary arguments)

    for(i = 0; i < 8 ; i++) {                                                   // -Implementing pencil and paper algorithm
        for(j = 0; j < 8; j++) {
            buff.uint64 = (uint64_t)a[i] * b[j] + result.uint32[i+j] + carriage;
            result.uint32[i+j] = buff.uint32[0];                                // -Equivalent to obtaining the modulus 2^32
            carriage = buff.uint32[1];                                          // -Equivalent to obtaining the quotient 2^32
        }
        result.uint32[i+j] = carriage;
        carriage = 0;
    }
    return result;
}

using namespace AES;

/*********************************************************************** Helper functions ************************************************************************/

static void operationModeToString(Key::OperationMode opm, char*const destination) {  // -Assuming destination is a pointer to an array of at least four elements.
    switch(opm) {
        case Key::ECB: strcpy(destination, "ECB");
            break;
        case Key::CBC: strcpy(destination, "CBC");
            break;
        case Key::PVS: strcpy(destination, "PVS");
            break;
    }
}

static int bytesToHexString(const char*const origin, char*const destination, const int len) { // -From an array of bytes creates a string witch is the
    if(len <= 0) return -1;                                                     //  representation in hexadecimal of that byte array
    char buff;                                                                  // -The resulting string is written on the "destination" pointer. We assume this
    int  i, j;
    for(i = 0, j = 0; i < len; i++) {                                           //  pointer points to an array with lengthBits at least 2*len+1
        buff = (char)((uint8_t)origin[i] >> 4);                                 // -Taking the four most significant bits
        if(buff<10) destination[j++] = buff + 48;                               // -To hexadecimal digit
        else        destination[j++] = buff + 55;                               //  ...
        buff = origin[i] & 15;                                                  // -Taking the four least significant byte
        if(buff<10) destination[j++] = buff + 48;                               // -To hexadecimal digit
        else        destination[j++] = buff + 55;                               //  ...
    }
    destination[j] = 0;                                                         // -End of string
    return 0;
}

/************************************************************** Handling AES cryptographic keys ******************************************************************/

Key::Key(): lengthBits(Length::_256), lengthBytes(32), operation_mode(OperationMode::CBC) {
    this->key = new char[this->lengthBytes];
    for(size_t i = 0; i < this->lengthBytes; i++) this->key[i] = 0;
}

Key::Key(Length len, OperationMode op_m): lengthBits(len), lengthBytes((size_t)len >> 3), operation_mode(op_m){
    std::random_device dev; std::mt19937 seed(dev());
    std::uniform_int_distribution<std::mt19937::result_type> distribution;      // -Random number with uniform distribution
    size_t i;
    union { int integer; char chars[4]; } buff;                                 // -Anonymous union. Casting from 32 bits integer to four chars
    this->key = new char[this->lengthBytes];
    for(i = 0; i < this->lengthBytes; i += 4) {                                 // -I am supposing everything is fine and lengthBytes is a multiple of four
        buff.integer = distribution(seed);                                      // -Taking a random 32 bits integer to divide it into four bytes
        memcpy((char*)(this->key + i), buff.chars, 4);
    }
}

Key::Key(const char* const _key, Length len, OperationMode op_m): lengthBits(len), lengthBytes((size_t)len >> 3), operation_mode(op_m){
    this->key = new char[this->lengthBytes];
    if(_key != NULL) for(size_t i = 0; i < this->lengthBytes; i++) this->key[i] = _key[i];
}

Key::Key(const Key& ak):lengthBits(ak.lengthBits), lengthBytes(ak.lengthBytes), operation_mode(ak.operation_mode) {
    size_t i;
    this->key = new char[ak.lengthBytes];
    for(i = 0; i < ak.lengthBytes; i++) this->key[i] = ak.key[i];               // -Supposing Cipher object is well constructed, this is, ak.key != NULL
    if(ak.operation_mode == CBC) {                                              // -Without CBC, copying IV is pointless.
        if((this->initializedIV = ak.initializedIV) == true)                    // -Copying and checking argument inside the 'if'
            for(i = 0; i < AES_BLK_SZ; i++) this->IV[i] = ak.IV[i];
    }
}

Key::Key(const char*const fname):lengthBits(_128), lengthBytes(AES_BLK_SZ), operation_mode(ECB) { // -Building from .key file
    char aeskeyStr[] = "AESKEY";
    char AESKEY[7];
    char opMode[4];
    uint16_t keyLen;
    size_t len_aeskeyStr = strlen(aeskeyStr);
    std::ifstream file;
    file.open(fname, std::ios::binary);
    if(file.is_open()) {
        file.read((char*)AESKEY, (std::streamsize)len_aeskeyStr);               // -Determining if file is a .key file
        AESKEY[6] = 0;                                                          // -End of string
        if(strcmp(AESKEY, aeskeyStr) == 0) {
                file.read((char*)opMode, 3);                                    // -Determining operation mode
                opMode[3] = 0;
                if(strcmp(opMode, "ECB") == 0) this->operation_mode = ECB;
                else if(strcmp(opMode, "CBC") == 0) this->operation_mode = CBC;
                else if(strcmp(opMode, "PVS") == 0) this->operation_mode = PVS;
                else {
                    std::cerr << "In file Source/AES.cpp, function Key::Key(const char*const fname):" << opMode << ", not a recognized operation mode.\n";
                    throw std::runtime_error("Not a recognized operation mode");
                }

                file.read((char*)&keyLen, 2);                                      // -Reading key lengthBits
                if(keyLen == 128 || keyLen == 192 || keyLen == 256) this->lengthBits = (Length)keyLen;
                else {
                    std::cerr << "In file Source/AES.cpp, function Key::Key(const char*const fname):" << keyLen << " is not a valid length in bits for key.\n";
                    throw std::runtime_error("Key length not allowed.");
                }
                this->lengthBytes = keyLen >> 3;                                // -lengthBytes = len / 8;

                this->key = new char[this->lengthBytes];                        // -Reading key
                file.read(this->key, (std::streamsize)this->lengthBytes);

                if(this->operation_mode == CBC) {
                    file.read((char*)this->IV, AES_BLK_SZ);                     // -In CBC case, reading IV.
                    this->initializedIV = true;
                }
           } else {
                std::cerr << "In file Source/AES.cpp, function Key::Key(const char*const fname): String " << fname << " does not represent a valid AES key file.\n";
                throw std::runtime_error("Not a valid AES key file.");
           }
    } else {
        std::cerr << "In file Source/AES.cpp, function Key::Key(const char*const fname): Failed to open " << fname << " file.\n";
        throw std::runtime_error("Could not open file.");
    }
}

Key::~Key() {
    if(this->key != NULL) delete[] this->key;
    this->key = NULL;
}

Key& Key::operator = (const Key& k) {
    if(this != &k) {                                                            // -Guarding against self assignment
        unsigned i;
        if(this->lengthBytes != k.lengthBytes) {                                // -Modifying length and array containing key only if necessary
            this->lengthBits = k.lengthBits;
            this->lengthBytes = k.lengthBytes;
            if(this->key != NULL) delete[] this->key;
            this->key = new char[k.lengthBytes];
        }
        for(i = 0; i < k.lengthBytes; i++) this->key[i] = k.key[i];
        this->operation_mode = k.operation_mode;
        if(k.operation_mode == CBC)                                             // -Without CBC, copying IV is pointless.
            if((this->initializedIV = k.initializedIV) == true)
                for(i = 0; i < AES_BLK_SZ; i++) this->IV[i] = k.IV[i];
    }
    return *this;
}

bool Key::operator == (const Key& k) const{
    unsigned i;
    if(this->lengthBytes != k.lengthBytes) return false;
    for(i = 0; i < this->lengthBytes; i++) if(this->key[i] != k.key[i]) return false;
    if(this->operation_mode == k.operation_mode) {
        if(this->operation_mode == OperationMode::CBC)
            for(i = 0; i < AES_BLK_SZ; i++) if(this->IV[i] != k.IV[i]) return false;
    } else return false;
    return true;
}

std::ostream& AES::operator << (std::ostream& ost, Key k) {
    char keyStr[65];
    char opModeStr[4];
    char IVstring[33];

    operationModeToString(k.operation_mode, opModeStr);
    bytesToHexString(k.key, keyStr, (int)k.lengthBytes);
    bytesToHexString(k.IV, IVstring, AES_BLK_SZ);

    ost << "\tKey size: " << k.lengthBits << " bits, " << k.lengthBytes << " bytes, Nk = " << (k.lengthBytes >> 2) << " words" << '\n';
    ost << "\tKey: " << keyStr << '\n';
    ost << "\tOperation mode: " << opModeStr << '\n';
    ost << "\tIV (in case of CBC): "<< IVstring << '\n';

    return ost;
}

void Key::set_IV(const char source[AES_BLK_SZ]) {
    if(!this->initializedIV) for(int i = 0; i < AES_BLK_SZ; i++) this->IV[i] = source[i];
    this->initializedIV = true;
}

void Key::save(const char* const fname) const {
    const char* aeskey = "AESKEY";                                              // File type.
    const char* op_mode= "ECB";
    switch(this->operation_mode) {                                              // -Operation mode.
        case ECB:
            op_mode = "ECB";
            break;
        case CBC:
            op_mode = "CBC";
            break;
        case PVS:
            op_mode = "PVS";
            break;
    }
    std::ofstream file;
    file.open(fname, std::ios::binary);
    if(file.is_open()) {
        file.write(aeskey,  6);                                                 // -File type
        file.write(op_mode, 3);                                                 // -Operation mode
        file.write((char*)&this->lengthBits, 2);                                // -Key lengthBits in bits
        file.write(this->key, (std::streamsize)this->lengthBytes);              // -Key
        if(this->operation_mode == CBC) file.write(this->IV, AES_BLK_SZ);       // -If CBC, writes initial vector
    } else {
        std::cerr << "In file Source/AES.cpp, function void Key::save(const char* const fname): Failed to write " << fname << " file.\n";
        throw std::runtime_error("File could not be written.");
    }
}


/************************************************** AES encryption and decryption algorithms implementation ******************************************************/

Cipher::PiRoundKey& Cipher::PiRoundKey::operator = (const PiRoundKey& prk) {
    if(this != &prk) {                                                          // -Guarding against self assignment
        size_t i;
        if(this->roundkey != NULL){
            delete[] this->roundkey;
            this->roundkey = NULL;
            this->size = 0;
        }
        if(prk.roundkey == NULL) return *this;
        this->roundkey = new char[prk.size];
        this->size = prk.size;
        for(i = 0; i < this->size; i++) this->roundkey[i] = prk.roundkey[i];
        for(i = 0; i < SBOX_SIZE; i++)  this->dinamicSbox[i] = prk.dinamicSbox[i];
        for(i = 0; i < SBOX_SIZE; i++)  this->dinamicSboxInv[i] = prk.dinamicSboxInv[i];
    }
    return *this;
}

void Cipher::PiRoundKey::setPiRoundKey(const Key &K) {
    if(this->roundkey != NULL) delete[] this->roundkey;
    std::ifstream file;
    file.open("pi.bin", std::ios::binary);
    if(!file.is_open()){
        file.close();
        file.open("Apps/Executables/pi.bin", std::ios::binary);
    }
    if(file.is_open()) {
        Uint256 a;                                                              // -Representation of a number of 256 bits (32 bytes)
        Uint256 b;                                                              // -Representation of a number of 256 bits (32 bytes)
        _16uint32_64uchar c;                                                    // -Will save the multiplication result
        unsigned i, j, k, piIndex;                                              // -piIndex will be used to go throw the pi array
        char _key_[MAX_KEY_LENGTH_BYTES];                                       // -Cryptographic key
        const size_t sizeof_c = sizeof(c.chars), sizeof__key_ = sizeof(_key_);
        const unsigned PIsize = 3*1024*1024, blockSize = 32;                    // -Size of pi array, size of the blocks we will divide pi array for its processing
        const unsigned _32bytesBlocks = PIsize >> 5;                            // -Amount of blocks of 32 bytes, _32bytesBlocks = PIsize / 32
        const unsigned lastBlockSize  = PIsize & 31;                            // -Size of the last block, lastBlockSize = dataSize % 32
        char* const pi = new char[PIsize];

        this->roundkey = new char[PIsize << 1];                                 // -PIsize << 1 == PIsize*2
        file.read(pi, PIsize);                                                  // -Uploading pi
        file.close();
        K.write_Key(_key_);                                                     // -Writing key on _key array
        if(K.getLengthBytes() < sizeof__key_)
            for(i = K.getLengthBytes(), j = 0; i < sizeof__key_; i++, j++)
                _key_[i] = _key_[j];                                            // -Padding with the beginning of the key

        a = Uint256(_key_);                                                     // -Creating 256 bits unsigned integer from _key_ array

        for(i = 0, piIndex = 0; i < _32bytesBlocks; i++, piIndex += blockSize) {
            b.reWriteLeastSignificantBytes(&pi[piIndex]);                       // -Number created from a 32 bytes chunk of pi
            c = a*b;                                                            // -Product with the number created from key
            for(j = 0 ; j < sizeof_c; j++)                                      // -Writing the result on roundkey array. Remember, the product of two 256 bits
                this->roundkey[this->size++] = (char)c.chars[j];                //  (32 bytes) numbers results on a 512 bits (64 bytes) number
        }
        if(lastBlockSize > 0) {
            b.reWriteLeastSignificantBytes(&pi[piIndex], lastBlockSize);        // -Rewriting with the bytes left
            c = a*b;                                                            // -Product with the key
            for(j = 0 ; j < lastBlockSize; j++)
                this->roundkey[this->size++] = (char)c.chars[j];
        }
        if(pi != NULL) delete[] pi;                                             // -Finishing with the creation of PiRoundKey

        size_t unwrittenSBoxSize = SBOX_SIZE ;                                  // -Creating dinamicSbox, this is the size of the entries not substituted yet
        uint8_t  permutationBuffer[SBOX_SIZE];                                  // -Will be used in the creation of new Sbox

        if(this->size < SBOX_SIZE)
            std::cerr <<
            "In Source/AES.cpp, function void Cipher::PiRoundKey::setPiRoundKey(const Key&). WARNING: PiRoundKey size < SBOX_SIZE.\n"
            "PiRoundKey size ==" << this->size << ", SBOX_SIZE = " << SBOX_SIZE << '\n';

        for(i = 0; i < SBOX_SIZE; i++ ) permutationBuffer[i] = (uint8_t)i;
            for(i = size-1, j = 0; unwrittenSBoxSize > 0; i--, j++, unwrittenSBoxSize--) {
                k = (uint8_t)this->roundkey[i] % unwrittenSBoxSize;
                this->dinamicSbox[j] = (char)permutationBuffer[k];
                permutationBuffer[k] = permutationBuffer[unwrittenSBoxSize - 1];
            }
        for(i = 0; i < SBOX_SIZE; i++ ) this->dinamicSboxInv[(uint32_t)(uint8_t)this->dinamicSbox[i]] = i; // -Building Sbox inverse
    } else {
        std::cerr << "Could not find/open pi.bin file.\n";
    }
}

void Cipher::PiRoundKey::subBytes(char state[AES_BLK_SZ]) const{
    for(int i = 0; i < AES_BLK_SZ; i++) state[i] = (char)this->dinamicSbox[(ui08)state[i]];
}

void Cipher::PiRoundKey::invSubBytes(char state[AES_BLK_SZ]) const{
    for(int i = 0; i < AES_BLK_SZ; i++) state[i] = (char)this->dinamicSboxInv[(ui08)state[i]];
}

Cipher::Cipher(): piRoundkey() {
    this->keyExpansion = new char[this->keyExpLen];
    for(int i = 0; i < this->keyExpLen; i++) this->keyExpansion[i] = 0;         // -Since the key constitutes of just zeros, key expansion is also just zeros
}

Cipher::Cipher(const Key& ak) :key(ak), Nk((int)ak.getLengthBytes() >> 2), Nr(Nk+6), keyExpLen((Nr+1)<<4), piRoundkey() {
    this->create_KeyExpansion(ak.key);
    if(this->key.operation_mode == Key::CBC) {
        if(!this->key.IVisInitialized()) {                                      // -In case of CBC, setting initial vector.
            char IVsource[AES_BLK_SZ];
            this->setAndWrite_IV(IVsource);
            this->key.set_IV(IVsource);
        }
    }
    if(this->key.operation_mode == Key::PVS) {                                  // -In case of PVS operation mode, initialize piRoundKey
        this->piRoundkey.setPiRoundKey(this->key);
    }
}

Cipher::Cipher(const Cipher& a) : key(a.key), Nk(a.Nk), Nr(a.Nr), keyExpLen(a.keyExpLen), piRoundkey() {
    this->keyExpansion = new char[(unsigned)a.keyExpLen];
    for(int i = 0; i < a.keyExpLen; i++) this->keyExpansion[i] = a.keyExpansion[i];
    this->piRoundkey = a.piRoundkey;
}

Cipher::~Cipher() {
    if(keyExpansion != NULL) delete[] keyExpansion;
    keyExpansion = NULL;
}

Cipher& Cipher::operator = (const Cipher& a) {
    if(this != &a) {
        this->key = a.key;
        if(this->keyExpLen != a.keyExpLen) {
            this->Nk = a.Nk;
            this->Nr = a.Nr;
            this->keyExpLen = a.keyExpLen;
            if(this->keyExpansion != NULL) delete[] keyExpansion;
            this->keyExpansion = new char[(unsigned)a.keyExpLen];
        }
        for(int i = 0; i < a.keyExpLen; i++) this->keyExpansion[i] = a.keyExpansion[i];
        this->piRoundkey = a.piRoundkey;
    }
    return *this;
}

std::ostream& AES::operator << (std::ostream& ost, const Cipher& c) {
    char keyExpansionString[880];
    int rowsAmount = c.keyExpLen >> 5;                                          // -rowsAmount = c.keyExpLen / 32
    int i, j, k, l;
    for(i = 0, j = 0, k = 0; i < rowsAmount; i++, j+=32, k+=64) {               // -Rows with 32 columns
        keyExpansionString[k++] = '\n';
        keyExpansionString[k++] = '\t';
        keyExpansionString[k++] = '\t';
        bytesToHexString(&c.keyExpansion[j], &keyExpansionString[k], 32);       // -Writing 32 bites pointed by &c.keyExpansion[j] in hexadecimal over 65 chars
    }                                                                           //  over &keyExpansionString[k]
    if((l = c.keyExpLen & 31) > 0) {                                            // -l = c.keyExpLen % 31
        keyExpansionString[k++] = '\n';
        keyExpansionString[k++] = '\t';
        keyExpansionString[k++] = '\t';
        bytesToHexString(&c.keyExpansion[j], &keyExpansionString[k], l);
        keyExpansionString[k+(l<<1)] = 0;                                       // -k+(l<<1) = k + l*2
    }
    else keyExpansionString[k] = 0;
    ost << "AES::Cipher object information:\n";
    ost << c.key;
    ost << "\tNr: " << c.Nr << " rounds" << '\n';
    ost << "\tKey Expansion size: " << c.keyExpLen << " bytes" << '\n';
    ost << "\tKey Expansion: " << keyExpansionString << '\n';
    return ost;
}

void Cipher::create_KeyExpansion(const char* const _key) {
    char temp[4];                                                               // (Nr+1)*16
	int i, keyExpansionLen = this->keyExpLen;                                   // Key expansion lengthBits in bytes
	keyExpansion = new char[(unsigned)keyExpansionLen];
	keyExpansionLen >>= 2;                                                      // keyExpansionLen in words (block of 4 bytes). keyExpansionLen /= 4
	int NkBytes = Nk << 2;                                                      // -The first Nk words of the key expansion are the key itself. // Nk * 4
	for(i = 0; i < NkBytes; i++) keyExpansion[i] = _key[i];

    bool debug = false;                                                         // -Show the construction of the key expansion.
	if(debug) {
	    std::cout <<
	    "-------------------------------------------------- Key Expansion --------------------------------------------------\n"
	    "-------------------------------------------------------------------------------------------------------------------\n"
	    "    |               |     After     |     After     |               |   After XOR   |               |     w[i] =   \n"
        " i  |     temp      |   RotWord()   |   SubWord()   |  Rcon[i/Nk]   |   with Rcon   |    w[i-Nk]    |   temp xor   \n"
        "    |               |               |               |               |               |               |    w[i-Nk]   \n"
        "-------------------------------------------------------------------------------------------------------------------\n";
	}

	for(i = Nk; i < keyExpansionLen; i++) {
		CopyWord(&(keyExpansion[(i - 1) << 2]), temp);                          // -Guarding against modify things that we don't want to modify.
        if(debug) {
            std::cout << " " << i;
            i < 10 ? std::cout << "  | " : std::cout << " | ";
		    printWord(temp);
        }
		if((i % Nk) == 0) {                                                     // -i is a multiple of Nk, witch value is 8
			RotWord(temp);
			if(debug) {
			    std::cout << " | ";
			    printWord(temp);
			}
			SubWord(temp);
			if(debug) {
			    std::cout << " | ";
			    printWord(temp);
			}
			if(debug) {
			    std::cout << " | ";
			    printWord(Rcon[i/Nk - 1]);
			}
			XORword(temp, Rcon[i/Nk -1], temp);
			if(debug) {
			    std::cout << " | ";
			    printWord(temp);
			}
		} else {
		    if(Nk > 6 && (i % Nk) == 4) {
		        if(debug) std::cout << " | ------------- | ";
			    SubWord(temp);
			    if(debug) {
			        printWord(temp);
			        std::cout << " | ------------- | -------------";
			    }
		    } else {
		        if(debug)
		            std::cout << " |               |               |               |              ";
		    }
		}
		if(debug) {
			std::cout << " | ";
			printWord(&(keyExpansion[(i - Nk) << 2]));
		}
		XORword(&(keyExpansion[(i - Nk) << 2]),temp, &(keyExpansion[i << 2]));
		if(debug) {
			std::cout << " | ";
			printWord(&(keyExpansion[i << 2]));
		}
		if(debug )std::cout << '\n';
	}
	if(debug) std::cout << "--------------------------------------------------"
	"-----------------------------------------------------------------\n\n";
	debug = false;
}

void Cipher::printWord(const char word[4]) {
    unsigned int temp = 0;
	std::cout << '[';
	for(int i = 0; i < 4; i++) {
	    temp = (ui08)0xFF & (ui08)word[i];
		if(temp < 16) std::cout << '0';
		printf("%X", temp);
		if(i != 3)
			std::cout << ",";
	}
	std::cout << ']';
}

void Cipher::encryptECB(char*const data, size_t size) const{
    if(size == 0)    return;
    if(data == NULL) return;

    char* currentDataBlock = data;
    int numofBlocks = int(size >> 4);                                           //  numofBlocks = size / 16.
    int rem = int(size & 15), i;                                                // -Bytes remaining rem = size % 16

    --numofBlocks;                                                              // Last block will be treated differently.
    for(i = 0; i < numofBlocks; i++) {
        encryptBlock(currentDataBlock);
        currentDataBlock += AES_BLK_SZ;
    }
    if(rem != 0) {                                                              // -This part of the code is for encrypt data that its size is not multiple of 16.
        encryptBlock(currentDataBlock);                                         //  This is not specified in the NIST standard.
        encryptBlock(currentDataBlock + rem);                                   // -Not handling the case size < 16
        return;
    }
    encryptBlock(currentDataBlock);
}

void Cipher::decryptECB(char *const data, size_t size) const{
    if(size == 0)    return;
    if(data == NULL) return;

    char* currentDataBlock = data;
    int numofBlocks = int(size >> 4);                                           //  numofBlocks = size / 16.
    int rem = int(size & 15), i;                                                // -Bytes remaining rem = size % 16

    --numofBlocks;                                                              // Last block will be treated differently.
    for(i = 0; i < numofBlocks; i++) {
        decryptBlock(currentDataBlock);
        currentDataBlock += AES_BLK_SZ;
    }
    if(rem != 0) {                                                              // -This part of the code is for encrypt data that its size is not multiple of 16.
        decryptBlock(currentDataBlock + rem);                                   //  This is not specified in the NIST standard.
        decryptBlock(currentDataBlock);                                         // -Not handling the case size < 16
        return;
    }
    decryptBlock(currentDataBlock);
}

void Cipher::setAndWrite_IV(char destination[AES_BLK_SZ]) const{                // -Simple method for setting the initial vector. The main idea is, when CBC is
    int_char ic; ic.int_ = time(NULL);                                         //  used, encryptBlock function encrypts a block of four consecutive 32 bits int's
    int icIntWordSize = sizeof(ic.int_);                                        // -At this moment, this is suppose to be equal to 4
    int j, k;
    for(k = 0; k < AES_BLK_SZ; ic.int_++)
        for(j = 0; j < icIntWordSize && k < AES_BLK_SZ; j++) destination[k++] = ic.chars[j];
    this->encryptBlock(destination);
}

void Cipher::encryptCBC(char*const data, size_t size) const{
    if(size == 0)    return;
    if(data == NULL) return;

    char *previousBlk, *currentDataBlock = data;
    char IVsource[AES_BLK_SZ];
    int numofBlocks = (int)size >> 4;                                           //  numofBlocks = size / 16.
    int rem = (int)size & 15, i;                                                // -Bytes remaining rem = size % 16

    this->key.write_IV(IVsource);
    XORblocks(currentDataBlock, IVsource, currentDataBlock);                    // -Encryption of the first block.
    encryptBlock(currentDataBlock);

    for(i = 1; i < numofBlocks; i++) {                                          // -Encryption of the rest of the blocks.
        previousBlk = currentDataBlock;
        currentDataBlock += AES_BLK_SZ;
        XORblocks(currentDataBlock, previousBlk, currentDataBlock);
        encryptBlock(currentDataBlock);
    }
    if(rem != 0) {                                                              // -This part of the code is for encrypt data that its size is not multiple of 16.
        previousBlk = currentDataBlock;                                         //  This is not specified in the NIST standard.
        currentDataBlock += AES_BLK_SZ;
        for(i=0; i<rem; i++) currentDataBlock[i] = currentDataBlock[i] ^ previousBlk[i];
        encryptBlock(previousBlk + rem);
    }
}


void Cipher::decryptCBC(char*const data, size_t size) const{
    if(size == 0)    return;
    if(data == NULL) return;

    char *currentDataBlock = data, previousBlk[AES_BLK_SZ], cipherCopy[AES_BLK_SZ];
    int numofBlocks = (int)size >> 4;                                           // -numofBlocks = size / 16
    int rem = (int)size & 15;                                                   // -Rest of the bytes rem = size % 16
    int i;

    this->key.write_IV(cipherCopy);
    CopyBlock(currentDataBlock, previousBlk);                                   // -Copying the first ciphered block.

    decryptBlock(currentDataBlock);                                             // -Deciphering the first block.
    XORblocks(currentDataBlock, cipherCopy, currentDataBlock);

    if(numofBlocks > 0) numofBlocks--;                                          // -Last block is going to be processed differently.
    for(i = 1; i < numofBlocks; i++) {                                          // -Decryption of the rest of the blocks.
        currentDataBlock += AES_BLK_SZ;
        CopyBlock(currentDataBlock, cipherCopy);                                // -Saving cipher block for the next round.
        decryptBlock(currentDataBlock);
        XORblocks(currentDataBlock, previousBlk, currentDataBlock);
        CopyBlock(cipherCopy, previousBlk);                                     // -Cipher block now becomes previous block.
    }
    currentDataBlock += AES_BLK_SZ;
    if(rem == 0) {                                                              // -Data size is a multiple of 16.
        decryptBlock(currentDataBlock);
        XORblocks(currentDataBlock, previousBlk, currentDataBlock);
    } else {                                                                    // -Data size isn't a multiple of 16.
        decryptBlock(currentDataBlock + rem);
        for(i = 0; i < rem; i++)
            currentDataBlock[i+AES_BLK_SZ] = currentDataBlock[i+AES_BLK_SZ] ^ currentDataBlock[i];
        decryptBlock(currentDataBlock);
        XORblocks(currentDataBlock, previousBlk, currentDataBlock);
    }
}

void Cipher::encryptPVS(char*const data, size_t size) const{
    if(!this->piRoundkey.roundKeyIsNULL()) {
        unsigned i, j;
        size_t piSize = this->piRoundkey.getSize();
        size_t size_blocks = size >> 4;                                         // -size_blocks = size / 16
        size_t last_blk_sz = size & 15;                                         // -last_blk_sz = size % 16
        for(i = 0, j = 0; i < size; i++, j++) {
            if(j == piSize) j = 0;
            data[i] ^= this->piRoundkey[j];
        }
        for(i = 0, j = 0; i < size_blocks; i++)
            this->piRoundkey.subBytes(data + (i << 4));                         // -data + i*16
        if(last_blk_sz > 0) this->piRoundkey.subBytes(data + (size - 16));      // -Handling case when data size is not a multiple of 16
        this->encryptECB(data, size);
    }
    else this->encryptECB(data, size);
}

void Cipher::decryptPVS(char*const data, size_t size) const{
    if(!this->piRoundkey.roundKeyIsNULL()) {
        unsigned i, j;
        size_t piSize = this->piRoundkey.getSize();
        size_t size_blocks = size >> 4;                                         // -size_blocks = size / 16
        size_t last_blk_sz = size & 15;                                         // -last_blk_sz = size % 16
        this->decryptECB(data, size);
        for(i = 0, j = 0; i < size_blocks; i++)
            this->piRoundkey.invSubBytes(data + (i << 4));                      // -data + i*16
        if(last_blk_sz > 0) this->piRoundkey.invSubBytes(data + (size - 16));
        for(i = 0, j = 0; i < size; i++, j++) {
            if(j == piSize) j = 0;
            data[i] ^= this->piRoundkey[j];
        }
    }
    else this->decryptECB(data, size);
}

void Cipher::encrypt(char*const data, size_t size) const{
    if(size == 0)    return;
    if(data == NULL) return;
    Key::OperationMode opMode = this->key.getOperationMode();
    switch(opMode) {
        case Key::ECB:
            this->encryptECB(data, size);
            break;
        case Key::CBC:
            this->encryptCBC(data, size);
            break;
        case Key::PVS:
            this->encryptPVS(data, size);
            break;
    }
}

void Cipher::decrypt(char*const data, size_t size) const{
    if(size == 0)    return;
    if(data == NULL) return;
    Key::OperationMode opMode = this->key.getOperationMode();
    switch(opMode) {
        case Key::ECB:
            this->decryptECB(data, size);
            break;
        case Key::CBC:
            this->decryptCBC(data, size);
            break;
        case Key::PVS:
            this->decryptPVS(data, size);
            break;
    }
}

void Cipher::XORblocks(char b1[AES_BLK_SZ], char b2[AES_BLK_SZ], char r[AES_BLK_SZ]) const {
    for(int i = 0; i < AES_BLK_SZ; i++) r[i] = b1[i] ^ b2[i];
}

void Cipher::printState(const char state[AES_BLK_SZ]) {
	int i, j, temp;
	for(i = 0; i < 4; i++) {
		std::cout << '[';
		for(j = 0; j < 4; j++) {
		    temp = (uint8_t)0xFF & (uint8_t)state[(j << 2) + i];
			if(temp < 16) std::cout << '0';
			printf("%X", temp);
			if(j != 3) std::cout << ", ";
		}
		std::cout << "]\n";
	}
}

void Cipher::CopyWord(const char source[4], char destination[4]) const {
    for(int i = 0; i < 4; i++) destination[i] = source[i];
}

void Cipher::CopyBlock(const char source[AES_BLK_SZ], char destination[AES_BLK_SZ]) const {
    for(int i = 0; i < AES_BLK_SZ; i++) destination[i] = source[i];
}

void Cipher::XORword(const char w1[4], const char w2[4], char resDest[4]) const{
    for(int i = 0; i < 4; i++) resDest[i] = w1[i] ^ w2[i];
}

void Cipher::RotWord(char word[4]) const{
    char temp = word[0]; int i;
	for(i = 0; i < 3; i++) word[i] = word[i + 1];
	word[3] = temp;
}

void Cipher::SubWord(char word[4]) const{
    for(int i = 0; i < 4; i++) word[i] = (char)SBox[(ui08)word[i]];
}

void Cipher::SubBytes(char state[AES_BLK_SZ]) const{                            // -Applies a substitution table (S-box) to each char.
	for(int i = 0; i < AES_BLK_SZ; i++) state[i] = (char)SBox[(ui08)state[i]];
}

void Cipher::ShiftRows(char state[AES_BLK_SZ]) const{                           // -Shift rows of the state array by different offset.
	int i, j; char tmp[4];
	for(i = 1; i < 4; i++) {
		for(j = 0; j < 4; j++) tmp[j] = state[i + (j << 2)];
		for(j = 0; j < 4; j++) state[i + (j << 2)] = tmp[(j + i) & 3];
	}
}

void Cipher::MixColumns(char state[AES_BLK_SZ]) const{                          // -Mixes the data within each column of the state array.
    int i, I, j, k;
	char temp[4];
	for(i = 0; i < 4; i++) {
	    I = i << 2;                                                             // I = i * 4;
		for(k = 0; k < 4; k++) temp[k] = state[I + k];                          // Copying row.
		for(j = 0; j < 4; j++) {
			state[I + j] = (char)multiply[ (int)(ui08)a[(4 - j) & 3] ][ (int)(ui08)temp[0] ]; // -First state column element times matrix first column
			for(k = 1; k < 4; k++) {
				state[I + j] ^= (char)multiply[ (int)(ui08)a[(k - j + 4)&3] ][ (int)(ui08)temp[k] ];
			}
		}
	}
}

void Cipher::AddRoundKey(char state[AES_BLK_SZ], int round) const{              // -Combines a round key with the state.
    round <<= 4;                                                                // -Each round uses 16 bytes and r <<= 4 == r *= 16.
	for(int i = 0; i < AES_BLK_SZ; i++) state[i] ^= keyExpansion[round + i];
}

void Cipher::encryptBlock(char block[]) const {
	int i, j;

	bool debug = false;                                                         // -True to show every encryption step.

	char *SOR, *ASB, *ASR, *AMC;                                                // -Debugging purposes.
	SOR = ASB = ASR = AMC = NULL;                                               // -Columns of the debugging table.

	if(debug) {                                                                 // (Nr + 2) * 16
        SOR = new char[(unsigned)(Nr + 2) << 4];
        AMC = new char[(unsigned)(Nr - 1) << 4];
        ASB = new char[(unsigned)Nr << 4];
        ASR = new char[(unsigned)Nr << 4];
	}

    if(debug) for(j = 0; j < AES_BLK_SZ; j++) SOR[j] = block[j];
	AddRoundKey(block, 0);
	if(debug) for(j = 0; j < AES_BLK_SZ; j++) SOR[j + AES_BLK_SZ] = block[j];

	for(i = 1; i < Nr; i++) {
		SubBytes(block);
		if(debug) for(j = 0; j < AES_BLK_SZ; j++) ASB[((i - 1) << 4) + j] = block[j];

		ShiftRows(block);
		if(debug) for(j = 0; j < AES_BLK_SZ; j++) ASR[((i - 1) << 4) + j] = block[j];

		MixColumns(block);
		if(debug) for(j = 0; j < AES_BLK_SZ; j++) AMC[((i - 1) << 4) + j] = block[j];

		AddRoundKey(block, i);
		if(debug) for(j = 0; j < AES_BLK_SZ; j++) SOR[((i + 1) << 4) + j] = block[j];
	}
	SubBytes(block);
	if(debug) for(j = 0; j < AES_BLK_SZ; j++) ASB[((i - 1) << 4) + j] = block[j];

	ShiftRows(block);
	if(debug) for(j = 0; j < AES_BLK_SZ; j++) ASR[((i - 1) << 4) + j] = block[j];

	AddRoundKey(block, i);
	if(debug) for(j = 0; j < AES_BLK_SZ; j++) SOR[((i + 1) << 4) + j] = block[j];

	if(debug) {
	    auto printBlockRow = [] (const char blk[AES_BLK_SZ], int row) {
	        unsigned int temp = 0;
            std::cout << '[';
	        for(int i = 0; i < AES_BLK_SZ; i += 4) {
	            temp = (ui08)0xFF & (ui08)blk[row + i];
		        if(temp < 16) std::cout << '0';
		        printf("%X", temp);
		        if(i != 12) std::cout << ",";
	        }
	        std::cout << ']';
	    };

	    std::cout <<
	    "---------------------------------------- Cipher ----------------------------------------\n"
	    "----------------------------------------------------------------------------------------\n"
	    " Round   |    Start of   |     After     |     After     |     After     |   Round key  \n"
        " Number  |     round     |    SubBytes   |   ShiftRows   |   MixColumns  |    value     \n"
        "         |               |               |               |               |              \n"
        "----------------------------------------------------------------------------------------\n";

        for(i = 0; i < 4; i++) {
            i == 1 ? std::cout << " "  << "input"  << "  ": std::cout << "        " ;
            std::cout << " | ";
            printBlockRow(SOR, i);
            std::cout << " |               |               |               | ";
            printBlockRow(keyExpansion, i);
            std::cout << '\n';
        }
        std::cout << '\n';

        for(i = 1; i <= Nr; i++) {
            for(j = 0; j < 4; j++) {
                if(j == 1) {
                    std::cout << "    ";
                    if(i < 10) std::cout << i  << "   ";
                    else std::cout << i  << "  ";
                }
                else std::cout << "        " ;

                std::cout << " | ";
                printBlockRow(&SOR[(i << 4)], j);
                std::cout << " | ";
                printBlockRow(&ASB[((i - 1) << 4)], j);
                std::cout << " | ";
                printBlockRow(&ASR[((i - 1) << 4)], j);
                std::cout << " | ";
                if(i < Nr) printBlockRow(&AMC[((i - 1) << 4)], j);
                else std::cout << "             ";
                std::cout << " | ";
                printBlockRow(&keyExpansion[(i << 4)], j);
                std::cout << '\n';
            }
            std::cout <<
            "----------------------------------------------------------------------------------------\n";
        }
        for(i = 0; i < 4; i++) {
            i == 1 ? std::cout << " "  << "output"  << " ": std::cout << "        " ;
            std::cout << " | ";
            printBlockRow(block, i);
            std::cout << " |               |               |               |               \n";
        }
        std::cout <<
        "----------------------------------------------------------------------------------------\n";
	}
    if(SOR != NULL) delete[] SOR;
    if(ASB != NULL) delete[] ASB;
    if(ASR != NULL) delete[] ASR;
    if(AMC != NULL) delete[] AMC;
    debug = false;
}

void Cipher::InvSubBytes(char state[AES_BLK_SZ]) const{
    for(int i = 0; i < AES_BLK_SZ; i++) state[i] = (char)invSBox[(ui08)state[i]];
}

void Cipher::InvShiftRows(char state[AES_BLK_SZ]) const{
    int i, j; char temp[4];
	for(i = 1; i < 4; i++) {
		for(j = 0; j < 4; j++) temp[j] = state[i + (j << 2)];
		for(j = 0; j < 4; j++) state[(j << 2) + i] = temp[(j - i + 4) & 3];
	}
}

void Cipher::InvMixColumns(char state[AES_BLK_SZ]) const{
    int  i, j, k, I;
	char temp[4];
	for(i = 0; i < 4; i ++) {
	    I = i << 2;                                                             // -I = i * 4
		for(j = 0; j < 4; j++) temp[j] = state[I + j];
		for(j = 0; j < 4; j++) {
			state[I + j] = (char)multiply[ (int)(ui08)aInv[(4 - j) & 0x03] ][ (int)(ui08)temp[0] ];
			for(k = 1; k < 4; k++)
			    state[I + j] ^= (char)multiply[ (int)(ui08)aInv[(k - j + 4)&3] ][ (int)(ui08)temp[k] ];
		}
	}
}

void Cipher::decryptBlock(char block[AES_BLK_SZ]) const {
    int i = Nr;
	AddRoundKey(block, i);
	for(i--; i > 0; i--) {
		InvShiftRows(block);
		InvSubBytes(block);
		AddRoundKey(block, i);
		InvMixColumns(block);
	}
	InvShiftRows(block);
	InvSubBytes(block);
	AddRoundKey(block, 0);
}
