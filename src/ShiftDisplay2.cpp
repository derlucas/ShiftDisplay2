/*
ShiftDisplay2
by Ameer Dawood
Arduino library for driving 7-segment displays using shift registers
https://ameer.io/ShiftDisplay2/
*/

#include "Arduino.h"
#include "ShiftDisplay2.h"
#include "CharacterEncoding.h"

// CONSTRUCTORS ****************************************************************

ShiftDisplay2::ShiftDisplay2(DisplayType displayType, int displaySize, DisplayDrive displayDrive) {
	int sectionSizes[] = {displaySize, 0}; // single section with size of display
	construct(DEFAULT_LATCH_PIN, DEFAULT_CLOCK_PIN, DEFAULT_DATA_PIN, displayType, sectionSizes, displayDrive, false, DEFAULT_INDEXES);
}

ShiftDisplay2::ShiftDisplay2(int latchPin, int clockPin, int dataPin, DisplayType displayType, int displaySize, DisplayDrive displayDrive) {
	int sectionSizes[] = {displaySize, 0}; // single section with size of display
	construct(latchPin, clockPin, dataPin, displayType, sectionSizes, displayDrive, false, DEFAULT_INDEXES);
}

ShiftDisplay2::ShiftDisplay2(DisplayType displayType, const int sectionSizes[], DisplayDrive displayDrive) {
	construct(DEFAULT_LATCH_PIN, DEFAULT_CLOCK_PIN, DEFAULT_DATA_PIN, displayType, sectionSizes, displayDrive, false, DEFAULT_INDEXES);
}

ShiftDisplay2::ShiftDisplay2(int latchPin, int clockPin, int dataPin, DisplayType displayType, const int sectionSizes[], DisplayDrive displayDrive) {
	construct(latchPin, clockPin, dataPin, displayType, sectionSizes, displayDrive, false, DEFAULT_INDEXES);
}

ShiftDisplay2::ShiftDisplay2(DisplayType displayType, int displaySize, bool swappedShiftRegisters, const int indexes[]) {
	int sectionSizes[] = {displaySize, 0}; // single section with size of display
	construct(DEFAULT_LATCH_PIN, DEFAULT_CLOCK_PIN, DEFAULT_DATA_PIN, displayType, sectionSizes, MULTIPLEXED_DRIVE, swappedShiftRegisters, indexes);
}

ShiftDisplay2::ShiftDisplay2(int latchPin, int clockPin, int dataPin, DisplayType displayType, int displaySize, bool swappedShiftRegisters, const int indexes[]) {
	int sectionSizes[] = {displaySize, 0}; // single section with size of display
	construct(latchPin, clockPin, dataPin, displayType, sectionSizes, MULTIPLEXED_DRIVE, swappedShiftRegisters, indexes);
}

ShiftDisplay2::ShiftDisplay2(DisplayType displayType, const int sectionSizes[], bool swappedShiftRegisters, const int indexes[]) {
	construct(DEFAULT_LATCH_PIN, DEFAULT_CLOCK_PIN, DEFAULT_DATA_PIN, displayType, sectionSizes, MULTIPLEXED_DRIVE, swappedShiftRegisters, indexes);
}

ShiftDisplay2::ShiftDisplay2(int latchPin, int clockPin, int dataPin, DisplayType displayType, const int sectionSizes[], bool swappedShiftRegisters, const int indexes[]) {
	construct(latchPin, clockPin, dataPin, displayType, sectionSizes, MULTIPLEXED_DRIVE, swappedShiftRegisters, indexes);
}

void ShiftDisplay2::construct(int latchPin, int clockPin, int dataPin, DisplayType displayType, const int sectionSizes[], DisplayDrive displayDrive, bool swappedShiftRegisters, const int indexes[]) {

	// initialize pins
	_latchPin = latchPin;
	_clockPin = clockPin;
	_dataPin = dataPin;
	pinMode(_latchPin, OUTPUT);
	pinMode(_clockPin, OUTPUT);
	pinMode(_dataPin, OUTPUT);

	// initialize globals
	_isCathode = displayType == COMMON_CATHODE;
	_isMultiplexed = displayDrive == MULTIPLEXED_DRIVE;
	_isSwapped = swappedShiftRegisters;

	// check and initialize indexes global
	for (int pos = 0; pos < MAX_DISPLAY_SIZE; pos++) {
		int index = indexes[pos];
		byte encodedIndex = 0;
		if (index >= 0 && index < MAX_DISPLAY_SIZE)
			encodedIndex = _isCathode ? ~INDEXES[index] : INDEXES[index];
		_indexes[pos] = encodedIndex;
	}

	// check and initialize size globals
	_displaySize = 0;
	_sectionCount = 0;
	int	sSize; // loop current section size
	while ((sSize = sectionSizes[_sectionCount]) > 0) {

		// check overflow
		if (_displaySize + sSize > MAX_DISPLAY_SIZE)
			break;

		_sectionBegins[_sectionCount] = _displaySize;
		_sectionSizes[_sectionCount] = sSize;
		_displaySize += sSize;
		_sectionCount++;
	}

	// clear cache and display
	byte empty = _isCathode ? EMPTY : ~EMPTY;
	memset(_cache, empty, MAX_DISPLAY_SIZE);
	clear();
}

// PRIVATE FUNCTIONS ***********************************************************

void ShiftDisplay2::updateMultiplexedDisplay() {
	for (int i = 0; i < _displaySize; i++) {
		digitalWrite(_latchPin, LOW);

		if (!_isSwapped) {
			shiftOut(_dataPin, _clockPin, LSBFIRST, _indexes[i]); // last shift register
			shiftOut(_dataPin, _clockPin, LSBFIRST, _cache[i]); // first shift register
		} else {
			shiftOut(_dataPin, _clockPin, LSBFIRST, _cache[i]); // last shift register
			shiftOut(_dataPin, _clockPin, LSBFIRST, _indexes[i]); // first shift register
		}

		digitalWrite(_latchPin, HIGH);

		delay(POV);
	}
}

void ShiftDisplay2::updateStaticDisplay() {
	digitalWrite(_latchPin, LOW);
	for (int i = _displaySize - 1; i >= 0 ; i--)
		shiftOut(_dataPin, _clockPin, LSBFIRST, _cache[i]);
	digitalWrite(_latchPin, HIGH);
}

void ShiftDisplay2::clearMultiplexedDisplay() {
	digitalWrite(_latchPin, LOW);
	shiftOut(_dataPin, _clockPin, MSBFIRST, EMPTY); // 0 at both ends of led
	shiftOut(_dataPin, _clockPin, MSBFIRST, EMPTY);
	digitalWrite(_latchPin, HIGH);
}

void ShiftDisplay2::clearStaticDisplay() {
	digitalWrite(_latchPin, LOW);
	byte empty = _isCathode ? EMPTY : ~EMPTY;
	for (int i = 0; i < _displaySize; i++)
		shiftOut(_dataPin, _clockPin, MSBFIRST, empty);
	digitalWrite(_latchPin, HIGH);
}

void ShiftDisplay2::modifyCache(int index, byte code) {
	_cache[index] = _isCathode ? code : ~code;
}

void ShiftDisplay2::modifyCache(int beginIndex, int size, const byte codes[]) {
	for (int i = 0; i < size; i++)
		_cache[i+beginIndex] = _isCathode ? codes[i] : ~codes[i];
}

void ShiftDisplay2::modifyCacheDot(int index, bool dot) {
	bool bit = _isCathode ? dot : !dot;
	bitWrite(_cache[index], 0, bit);
}

void ShiftDisplay2::encodeCharacters(int size, const char input[], byte output[], int dotIndex = -1) {
	for (int i = 0; i < size; i++) {
		char c = input[i];
		
		if (c >= '0' && c <= '9')
			output[i] = NUMBERS[c - '0'];
		else if (c >= 'a' && c <= 'z')
			output[i] = LETTERS[c - 'a'];
		else if (c >= 'A' && c <= 'Z')
			output[i] = LETTERS[c - 'A'];
		else if (c == '-')
			output[i] = MINUS;
		else // space or invalid
			output[i] = EMPTY;
	}
	
	if (dotIndex != -1)
		bitWrite(output[dotIndex], 0, 1);
}

int ShiftDisplay2::formatCharacters(int inSize, const char input[], int outSize, char output[], Alignment alignment, bool leadingZeros = false, int decimalPlaces = -1) {
	
	// index of character virtual borders
	int left; // lowest index
	int right; // highest index

	int minimum = 0; // minimum display index possible
	int maximum = outSize - 1; // maximum display index possible

	// calculate borders according to alignment
	if (alignment == ALIGN_LEFT) {
		left = minimum;
		right = inSize - 1;
	} else if (alignment == ALIGN_RIGHT) {
		left = outSize - inSize;
		right = maximum;
	} else { // ALIGN_CENTER:
		left = (outSize - inSize) / 2;
		right = left + inSize - 1;
	}
	
	// fill output array with empty space or characters
	for (int i = 0; i < left; i++) // before characters
		output[i] = leadingZeros ? '0' : ' ';
	for (int i = left, j = 0; i <= right; i++, j++) // characters
		if (i >= minimum && i <= maximum) // not out of bounds on display
			output[i] = input[j];
	for (int i = right+1; i < outSize; i++) // after characters
		output[i] = ' ';

	// calculate dot index and return it or -1 if none
	if (decimalPlaces == -1)
		return -1;
	int dotIndex = right - decimalPlaces;
	if (dotIndex < minimum || dotIndex > maximum) // out of bounds
		return -1;
	return dotIndex;
}

void ShiftDisplay2::getCharacters(long input, int size, char output[]) {

	// invert negative
	bool negative = false;
	if (input < 0) {
		negative = true;
		input = -input;
	}

	// iterate every array position, even if all zeros
	for (int i = size-1; i >= 0; i--) {
		int digit = input % 10;
		char c = digit + '0';
		output[i] = c;
		input /= 10;
	}

	// insert a minus character if negative
	if (negative)
		output[0] = '-';
}

int ShiftDisplay2::countCharacters(long number) {
	if (number < 0) // negative number
		return 1 + countCharacters(-number); // minus counts as a character
	if (number < 10)
		return 1;
	return 1 + countCharacters(number / 10);
}

int ShiftDisplay2::countCharacters(double number) {
	if (number > -1 && number < 0) // -0.x
		return 2; // minus and zero count as 2 characters
	return countCharacters((long) number);
}

void ShiftDisplay2::setInteger(long number, bool leadingZeros, Alignment alignment, int section) {
	int valueSize = countCharacters(number);
	char originalCharacters[valueSize];
	getCharacters(number, valueSize, originalCharacters);
	int sectionSize = _sectionSizes[section];
	char formattedCharacters[sectionSize];
	formatCharacters(valueSize, originalCharacters, sectionSize, formattedCharacters, alignment, leadingZeros);
	byte encodedCharacters[sectionSize];
	encodeCharacters(sectionSize, formattedCharacters, encodedCharacters);
	modifyCache(_sectionBegins[section], sectionSize, encodedCharacters);
}

void ShiftDisplay2::setReal(double number, int decimalPlaces, bool leadingZeros, Alignment alignment, int section) {
	long value = round(number * pow(10, decimalPlaces)); // convert to integer (eg 1.236, 2 = 124)
	int valueSize = countCharacters(number) + decimalPlaces;
	char originalCharacters[valueSize];
	getCharacters(value, valueSize, originalCharacters);
	int sectionSize = _sectionSizes[section];
	char formattedCharacters[sectionSize];
	int dotIndex = formatCharacters(valueSize, originalCharacters, sectionSize, formattedCharacters, alignment, leadingZeros, decimalPlaces);
	byte encodedCharacters[sectionSize];
	encodeCharacters(sectionSize, formattedCharacters, encodedCharacters, dotIndex);
	modifyCache(_sectionBegins[section], sectionSize, encodedCharacters);
}

void ShiftDisplay2::setNumber(long number, int decimalPlaces, bool leadingZeros, Alignment alignment, int section = 0) {
	if (decimalPlaces == 0)
		setInteger(number, leadingZeros, alignment, section);
	else
		setReal(number, decimalPlaces, leadingZeros, alignment, section);
}

void ShiftDisplay2::setNumber(double number, int decimalPlaces, bool leadingZeros, Alignment alignment, int section = 0) {
	if (decimalPlaces == 0) {
		long roundNumber = round(number);
		setInteger(roundNumber, leadingZeros, alignment, section);
	} else
		setReal(number, decimalPlaces, leadingZeros, alignment, section);
}

void ShiftDisplay2::setText(char value, Alignment alignment, int section = 0) {
	char originalCharacters[] = {value};
	int sectionSize = _sectionSizes[section];
	char formattedCharacters[sectionSize];
	formatCharacters(1, originalCharacters, sectionSize, formattedCharacters, alignment);
	byte encodedCharacters[sectionSize];
	encodeCharacters(sectionSize, formattedCharacters, encodedCharacters);
	modifyCache(_sectionBegins[section], sectionSize, encodedCharacters);
}

void ShiftDisplay2::setText(const char value[], Alignment alignment, int section = 0) {
	int valueSize = strlen(value);
	int sectionSize = _sectionSizes[section];
	char formattedCharacters[sectionSize];
	formatCharacters(valueSize, value, sectionSize, formattedCharacters, alignment);
	byte encodedCharacters[sectionSize];
	encodeCharacters(sectionSize, formattedCharacters, encodedCharacters);
	modifyCache(_sectionBegins[section], sectionSize, encodedCharacters);
}

void ShiftDisplay2::setText(const String &value, Alignment alignment, int section = 0) {
	
	// convert String to char array manually for better support between Arduino cores
	int size = 0;
	while (value[size] != '\0')
		size++;
	char str[size + 1];
	for (int i = 0; i < size; i++)
		str[i] = value[i];
	str[size] = '\0';

	setText(str, alignment, section); // call char array function
}

bool ShiftDisplay2::isValidSection(int section) {
	return section >= 0 && section < _sectionCount;
}

// PUBLIC FUNCTIONS ************************************************************

void ShiftDisplay2::set(int number, int decimalPlaces, bool leadingZeros, Alignment alignment) {
	setNumber((long)number, decimalPlaces, leadingZeros, alignment);
}

void ShiftDisplay2::set(int number, bool leadingZeros, Alignment alignment) {
	setNumber((long)number, DEFAULT_DECIMAL_PLACES_INTEGER, leadingZeros, alignment);
}

void ShiftDisplay2::set(int number, int decimalPlaces, Alignment alignment) {
	setNumber((long)number, decimalPlaces, DEFAULT_LEADING_ZEROS, alignment);
}

void ShiftDisplay2::set(int number, Alignment alignment) {
	setNumber((long)number, DEFAULT_DECIMAL_PLACES_INTEGER, DEFAULT_LEADING_ZEROS, alignment);
}

void ShiftDisplay2::set(long number, int decimalPlaces, bool leadingZeros, Alignment alignment) {
	setNumber(number, decimalPlaces, leadingZeros, alignment);
}

void ShiftDisplay2::set(long number, bool leadingZeros, Alignment alignment) {
	setNumber(number, DEFAULT_DECIMAL_PLACES_INTEGER, leadingZeros, alignment);
}

void ShiftDisplay2::set(long number, int decimalPlaces, Alignment alignment) {
	setNumber(number, decimalPlaces, DEFAULT_LEADING_ZEROS, alignment);
}

void ShiftDisplay2::set(long number, Alignment alignment) {
	setNumber(number, DEFAULT_DECIMAL_PLACES_INTEGER, DEFAULT_LEADING_ZEROS, alignment);
}

void ShiftDisplay2::set(double number, int decimalPlaces, bool leadingZeros, Alignment alignment) {
	setNumber(number, decimalPlaces, leadingZeros, alignment);
}

void ShiftDisplay2::set(double number, bool leadingZeros, Alignment alignment) {
	setNumber(number, DEFAULT_DECIMAL_PLACES_REAL, leadingZeros, alignment);
}

void ShiftDisplay2::set(double number, int decimalPlaces, Alignment alignment) {
	setNumber(number, decimalPlaces, DEFAULT_LEADING_ZEROS, alignment);
}

void ShiftDisplay2::set(double number, Alignment alignment) {
	setNumber(number, DEFAULT_DECIMAL_PLACES_REAL, DEFAULT_LEADING_ZEROS, alignment);
}

void ShiftDisplay2::set(char value, Alignment alignment) {
	setText(value, alignment);
}

void ShiftDisplay2::set(const char value[], Alignment alignment) {
	setText(value, alignment);
}

void ShiftDisplay2::set(const String &value, Alignment alignment) {
	setText(value, alignment);
}

void ShiftDisplay2::set(const byte customs[]) {
	setAt(0, customs);
}

void ShiftDisplay2::set(const char characters[], const bool dots[]) {
	setAt(0, characters, dots);
}

void ShiftDisplay2::setAt(int section, int number, int decimalPlaces, bool leadingZeros, Alignment alignment) {
	if (isValidSection(section))
		setNumber((long)number, decimalPlaces, leadingZeros, alignment, section);
}

void ShiftDisplay2::setAt(int section, int number, bool leadingZeros, Alignment alignment) {
	if (isValidSection(section))
		setNumber((long)number, DEFAULT_DECIMAL_PLACES_INTEGER, leadingZeros, alignment, section);
}

void ShiftDisplay2::setAt(int section, int number, int decimalPlaces, Alignment alignment) {
	if (isValidSection(section))
		setNumber((long)number, decimalPlaces, DEFAULT_LEADING_ZEROS, alignment, section);
}

void ShiftDisplay2::setAt(int section, int number, Alignment alignment) {
	if (isValidSection(section))
		setNumber((long)number, DEFAULT_DECIMAL_PLACES_INTEGER, DEFAULT_LEADING_ZEROS, alignment, section);
}

void ShiftDisplay2::setAt(int section, long number, int decimalPlaces, bool leadingZeros, Alignment alignment) {
	if (isValidSection(section))
		setNumber(number, decimalPlaces, leadingZeros, alignment, section);
}

void ShiftDisplay2::setAt(int section, long number, bool leadingZeros, Alignment alignment) {
	if (isValidSection(section))
		setNumber(number, DEFAULT_DECIMAL_PLACES_INTEGER, leadingZeros, alignment, section);
}

void ShiftDisplay2::setAt(int section, long number, int decimalPlaces, Alignment alignment) {
	if (isValidSection(section))
		setNumber(number, decimalPlaces, DEFAULT_LEADING_ZEROS, alignment, section);
}

void ShiftDisplay2::setAt(int section, long number, Alignment alignment) {
	if (isValidSection(section))
		setNumber(number, DEFAULT_DECIMAL_PLACES_INTEGER, DEFAULT_LEADING_ZEROS, alignment, section);
}

void ShiftDisplay2::setAt(int section, double number, int decimalPlaces, bool leadingZeros, Alignment alignment) {
	if (isValidSection(section))
		setNumber(number, decimalPlaces, leadingZeros, alignment, section);
}

void ShiftDisplay2::setAt(int section, double number, bool leadingZeros, Alignment alignment) {
	if (isValidSection(section))
		setNumber(number, DEFAULT_DECIMAL_PLACES_REAL, leadingZeros, alignment, section);
}

void ShiftDisplay2::setAt(int section, double number, int decimalPlaces, Alignment alignment) {
	if (isValidSection(section))
		setNumber(number, decimalPlaces, DEFAULT_LEADING_ZEROS, alignment, section);
}

void ShiftDisplay2::setAt(int section, double number, Alignment alignment) {
	if (isValidSection(section))
		setNumber(number, DEFAULT_DECIMAL_PLACES_REAL, DEFAULT_LEADING_ZEROS, alignment, section);
}

void ShiftDisplay2::setAt(int section, char value, Alignment alignment) {
	if (isValidSection(section))
		setText(value, alignment, section);
}

void ShiftDisplay2::setAt(int section, const char value[], Alignment alignment) {
	if (isValidSection(section))
		setText(value, alignment, section);
}

void ShiftDisplay2::setAt(int section, const String &value, Alignment alignment) {
	if (isValidSection(section))
		setText(value, alignment, section);
}

void ShiftDisplay2::setAt(int section, const byte customs[]) {
	if (isValidSection(section)) {
		int sectionSize = _sectionSizes[section];
		modifyCache(_sectionBegins[section], sectionSize, (byte*) customs);
	}
}

void ShiftDisplay2::setAt(int section, const char characters[], const bool dots[]) {
	if (isValidSection(section)) {
		int sectionSize = _sectionSizes[section];
		byte encodedCharacters[sectionSize];
		encodeCharacters(sectionSize, characters, encodedCharacters);
		int begin = _sectionBegins[section];
		modifyCache(begin, sectionSize, encodedCharacters);
		for (int i = 0; i < sectionSize; i++)
			modifyCacheDot(i+begin, dots[i]);
	}
}

void ShiftDisplay2::changeDot(int index, bool dot) {
	changeDotAt(0, index, dot);
}

void ShiftDisplay2::changeCharacter(int index, byte custom) {
	changeCharacterAt(0, index, custom);
}

void ShiftDisplay2::changeDotAt(int section, int relativeIndex, bool dot) {
	if (isValidSection(section)) {
		if (relativeIndex >= 0 && relativeIndex < _sectionSizes[section]) { // valid index in display
			int index = _sectionBegins[section] + relativeIndex;
			modifyCacheDot(index, dot);
		}
	}
}

void ShiftDisplay2::changeCharacterAt(int section, int relativeIndex, byte custom) {
	if (isValidSection(section)) {
		if (relativeIndex >= 0 && relativeIndex < _sectionSizes[section]) { // valid index in display
			int index = _sectionBegins[section] + relativeIndex;
			modifyCache(index, custom);
		}
	}
}

void ShiftDisplay2::update() {
	if (_isMultiplexed)
		updateMultiplexedDisplay();
	else
		updateStaticDisplay();
}

void ShiftDisplay2::clear() {
	if (_isMultiplexed)
		clearMultiplexedDisplay();
	else
		clearStaticDisplay();
}

void ShiftDisplay2::show(unsigned long time) {
	if (_isMultiplexed) {
		unsigned long beforeLast = millis() + time - (POV * _displaySize); // start + total - last iteration
		while (millis() <= beforeLast) // it will not enter loop if it would overtake time
			updateMultiplexedDisplay();
		clearMultiplexedDisplay();
	} else {
		updateStaticDisplay();
		delay(time);
		clearStaticDisplay();
	}
}

void ShiftDisplay2::scroll(String &value, int speed) {
	scroll(value, speed);
}

void ShiftDisplay2::scroll(const char value[], int speed) {
	scroll(value, speed);
}

void ShiftDisplay2::scroll(char value, int speed) {
	String _value = String(value);
	int _endChar = _displaySize;
	if (_value.length() > _displaySize) {
		for (int i = 0; i < _value.length(); i++) {
			set(_value.substring(i, _endChar));
			show(speed);
			_endChar = _endChar + 1;
		}
	} else {
		set(_value);
		show(speed * _displaySize);
	}
}


// DEPRECATED ******************************************************************
void ShiftDisplay2::insertPoint(int index) { modifyCacheDot(index, true); }
void ShiftDisplay2::removePoint(int index) { modifyCacheDot(index, false); }
void ShiftDisplay2::insertDot(int index) { modifyCacheDot(index, true); }
void ShiftDisplay2::removeDot(int index) { modifyCacheDot(index, false); }
void ShiftDisplay2::print(long time, int value, Alignment alignment) { show(value, time, alignment); }
void ShiftDisplay2::print(long time, long value, Alignment alignment) { show(value, time, alignment); }
void ShiftDisplay2::print(long time, double value, int decimalPlaces, Alignment alignment) { show(value, time, decimalPlaces, alignment); }
void ShiftDisplay2::print(long time, double value, Alignment alignment) { show(value, time, alignment); }
void ShiftDisplay2::print(long time, char value, Alignment alignment) { show(value, time, alignment); }
void ShiftDisplay2::print(long time, const char value[], Alignment alignment) { show(value, time, alignment); }
void ShiftDisplay2::print(long time, const String &value, Alignment alignment) { show(value, time, alignment); }
void ShiftDisplay2::show() { updateMultiplexedDisplay(); clearMultiplexedDisplay(); }
void ShiftDisplay2::show(int value, unsigned long time, Alignment alignment) { set(value, alignment); show(time); }
void ShiftDisplay2::show(long value, unsigned long time, Alignment alignment) { set(value, alignment); show(time); }
void ShiftDisplay2::show(double valueReal, unsigned long time, int decimalPlaces, Alignment alignment) { set(valueReal, decimalPlaces, alignment); show(time); }
void ShiftDisplay2::show(double valueReal, unsigned long time, Alignment alignment) { set(valueReal, alignment); show(time); }
void ShiftDisplay2::show(char value, unsigned long time, Alignment alignment) { set(value, alignment); show(time); }
void ShiftDisplay2::show(const char value[], unsigned long time, Alignment alignment) { set(value, alignment); show(time); }
void ShiftDisplay2::show(const String &value, unsigned long time, Alignment alignment) { set(value, alignment); show(time); }
void ShiftDisplay2::show(const byte customs[], unsigned long time) { set(customs); show(time); }
void ShiftDisplay2::show(const char characters[], const bool dots[], unsigned long time) { set(characters, dots); show(time); }
ShiftDisplay2::ShiftDisplay2(DisplayType displayType, int sectionCount, const int sectionSizes[]) { ShiftDisplay2(DEFAULT_LATCH_PIN, DEFAULT_CLOCK_PIN, DEFAULT_DATA_PIN, displayType, sectionCount, sectionSizes); }
ShiftDisplay2::ShiftDisplay2(int latchPin, int clockPin, int dataPin, DisplayType displayType, int sectionCount, const int sectionSizes[]) { int s[sectionCount+1]; s[sectionCount] = 0; memcpy(s, sectionSizes, sectionCount*sizeof(int)); ShiftDisplay2(latchPin, clockPin, dataPin, displayType, s, MULTIPLEXED_DRIVE); }
void ShiftDisplay2::setDot(int index, bool dot) { changeDot(index, dot); }
void ShiftDisplay2::setDotAt(int section, int relativeIndex, bool dot) { changeDotAt(section, relativeIndex, dot); }
void ShiftDisplay2::setCustom(int index, byte custom) { changeCharacter(index, custom); }
void ShiftDisplay2::setCustomAt(int section, int relativeIndex, byte custom) { changeCharacterAt(section, relativeIndex, custom); }
