#include <Arduino.h>
#include <math.h>

const int serverPin = 13;

enum HandShakeState {
    CSTART, CWAITACK, SLISTEN, SWAITKEY, SWAITACK, SWAITKEY2, SWAITACK2, DATAEXCHANGE
};
// Part 1 code
/*
    Returns true if arduino is server, false if arduino is client
*/
bool isServer() {
    if (digitalRead(serverPin) == HIGH) {
        return true;
    } else {
        return false;
    }
}

/*
    Compute and return (a*b)%m
    Note: m must be less than 2^31
    Arguments:
        a (uint32_t): The first multiplicant
        b (uint32_t): The second multiplicant
        m (uint32_t): The mod value
    Returns:
        result (uint32_t): (a*b)%m
*/
uint32_t multMod(uint32_t a, uint32_t b, uint32_t m) {
    uint32_t result = 0;
    uint32_t dblVal = a%m;
    uint32_t newB = b;

    // This is the result of working through the worksheet.
    // Notice the extreme similarity with powmod.
    while (newB > 0) {
        if (newB & 1) {
            result = (result + dblVal) % m;
        }
        dblVal = (dblVal << 1) % m;
        newB = (newB >> 1);
    }

    return result;
}


/*
    NOTE: This was modified using our multMod function, but is otherwise the
    function powModFast provided in the lectures.

    Compute and return (a to the power of b) mod m.
	  Example: powMod(2, 5, 13) should return 6.
*/
uint32_t powMod(uint32_t a, uint32_t b, uint32_t m) {
    uint32_t result = 1 % m;
    uint32_t sqrVal = a % m;  // stores a^{2^i} values, initially 2^{2^0}
    uint32_t newB = b;

    // See the lecture notes for a description of why this works.
    while (newB > 0) {
        // evalutates to true iff i'th bit of b is 1 in the i'th iteration
        if (newB & 1) {
            result = multMod(result, sqrVal, m);
        }
        sqrVal = multMod(sqrVal, sqrVal, m);
        newB = (newB >> 1);
    }

    return result;
}

/** Writes an uint32_t to Serial3, starting from the least-significant
 * and finishing with the most significant byte.
 */
void uint32_to_serial3(uint32_t num) {
    Serial3.write((char) (num >> 0));
    Serial3.write((char) (num >> 8));
    Serial3.write((char) (num >> 16));
    Serial3.write((char) (num >> 24));
}


/** Reads an uint32_t from Serial3, starting from the least-significant
*   and finishing with the most significant byte.
 */
uint32_t uint32_from_serial3() {
    uint32_t num = 0;
    num = num | ((uint32_t) Serial3.read()) << 0;
    num = num | ((uint32_t) Serial3.read()) << 8;
    num = num | ((uint32_t) Serial3.read()) << 16;
    num = num | ((uint32_t) Serial3.read()) << 24;
    return num;
}


/*
    Encrypts using RSA encryption.

    Arguments:
        c (char): The character to be encrypted
        e (uint32_t): The partner's public key
        m (uint32_t): The partner's modulus

    Return:
        The encrypted character (uint32_t)
*/
uint32_t encrypt(char c, uint32_t e, uint32_t m) {
    return powMod(c, e, m);
}


/*
    Decrypts using RSA encryption.

    Arguments:
        x (uint32_t): The communicated integer
        d (uint32_t): The Arduino's private key
        n (uint32_t): The Arduino's modulus

    Returns:
        The decrypted character (char)
*/
char decrypt(uint32_t x, uint32_t d, uint32_t n) {
    return (char) powMod(x, d, n);
}

/*
    Core communication loop
    d, n, e, and m are according to the assignment spec
*/
void communication(uint32_t d, uint32_t n, uint32_t e, uint32_t m) {
    // Consume all early content from Serial3 to prevent garbage communication
    while (Serial3.available()) {
        Serial3.read();
    }

    // Enter the communication loop
    while (true) {
        // Check if the other Arduino sent an encrypted message.
        if (Serial3.available() >= 4) {
            // Read in the next character, decrypt it, and display it
            uint32_t read = uint32_from_serial3();
            Serial.print(decrypt(read, d, n));
        }

        // Check if the user entered a character.
        if (Serial.available() >= 1) {
            char byteRead = Serial.read();
            // Read the character that was typed, echo it to the serial monitor,
            // and then encrypt and transmit it.
            if ((int) byteRead == '\r') {
                // If the user pressed enter, we send both '\r' and '\n'
                Serial.print('\r');
                uint32_to_serial3(encrypt('\r', e, m));
                Serial.print('\n');
                uint32_to_serial3(encrypt('\n', e, m));
            } else {
                Serial.print(byteRead);
                uint32_to_serial3(encrypt(byteRead, e, m));
            }
        }
    }
}


/*
    Performs basic Arduino setup tasks.
*/
void setup() {
    init();
    Serial.begin(9600);
    Serial3.begin(9600);

    Serial.println("Welcome to Arduino Chat!");
}

//Part 2 code

/** Waits  for a certain  number  of  bytes on  Serial3  or  timeout
* @param  nbytes: the  number  of  bytes we want
* @param  timeout: timeout  period (ms); specifying a negative  number
*               turns  off  timeouts (the  function  waits  indefinitely
*               if  timeouts  are  turned  off).
* @return  True if the  required  number  of bytes  have  arrived.
*/
bool  wait_on_serial3(uint8_t nbytes, long timeout) {
    unsigned  long  deadline = millis() + timeout;
    // wraparound  not a problem
    while (Serial3.available() < nbytes  && (timeout < 0 ||  millis() < deadline)) {
    delay(1);  // be nice , no busy loop
    }
    return  Serial3.available () >=nbytes;
}
/*
    RandKbit finds a random k- bit integer by using analogRead() k times and saving
        the least significant bit k.
    arguments: int k, the amount of bits needed for the random number
    return: uint16_t k_bit, k bit random number. 
*/
uint32_t RandKbit(int k) {
    uint16_t k_bit = 0;
    bool k_bitloop = true;
    while (k_bitloop) {
        for (int i = 1; i <= k; i++) {
            uint16_t analognum = analogRead(1);
            delay(5);
            if ((analognum % 2) == 0) {
                k_bit = (k_bit << 1ul);
            } else {
                k_bit = (k_bit << 1ul);
                k_bit = k_bit | 1;
            }
        }
        if ((pow(2, k) <= k_bit) && (k_bit < pow(2, k+1))) {
            k_bitloop = false;
        }
    }
    return k_bit;
}
/*
    returns the greatest common divisor of n and p, assumes n>0;
    arguments: uint32_t n, a 32 bit number
               uint32_t p, a 32 bit number
    returns: uint32_t n, a 32 bit number, which is the gcd of n and p;
*/
uint32_t gcd(uint32_t n, uint32_t p) {
    uint32_t temp;
    while (p > 0) {
        n %= p;
        temp = n;
        n = p;
        p = temp;
    }
    return n;
}
/*
    Primality() checks if k_bit is a prime and is between 2^k and 2^(k+1)-1
    arguments: uint32_t k_bit, a k bit number
                int k, amount of bits in k_bit
    returns:    uint32_t k_bit, if k_bit is not a prime or not within range
                , k_bit will be incremented until it is.
*/
uint32_t Primality(uint32_t k_bit, int k) {
    bool loop = true;
    bool prime = true;
    while (loop) {
        prime = true;
        for (int i = 2; i < sqrt(k_bit); i++) {
            if (k_bit % i == 0) {
                prime = false;
            }
        }
        if ((prime == true)) {
                loop = false;
        } else {
            k_bit++;
            if (k_bit == pow(2, k+1)-1) {
                k_bit = pow(2, k);
            }
        }
    }
    return k_bit;
}

/*
    generates two prime numbers and calculates n and phin
    arguments: uint32_t& n, referenced arduinos modulus
    returns:    uint32_t phin, the totient of n;
*/
uint32_t generateN(uint32_t& n) {
    uint32_t p, q;
    uint32_t phin;
    p = RandKbit(14);
    q = RandKbit(15);
    p = Primality(p, 14);
    q = Primality(q, 15);
    n = p*q;

    phin = (p-1)*(q-1);
    return phin;
}

/*
    generates d using Euclid's Extended Algorithm
    arguments: uint32_t e,
                uint32_t phin,
    returns: - uint32_t s[i-1] if s[i-1] is between 1 and phin
             - 0 if s[i-1] is not between 1 and phin or dvalue is is not 1
*/
uint32_t generateD(uint32_t e, uint32_t phin) {
    int32_t r[100];
    int32_t s[100], t[100];
    int dvalue, q, i = 1;
    r[0] = e;
    r[1] = phin;
    s[0] = 1;
    s[1] = 0;
    t[0] = 0;
    t[1] = 1;
    while (r[i] > 0) {
        q = r[i-1]/r[i];
        r[i+1] = r[i-1] - q*r[i];
        s[i+1] = s[i-1] - q*s[i];
        t[i+1] = t[i-1] - q*t[i];
        i++;
    }
    dvalue = r[0]*s[i-1] + r[1]*t[i-1];
    if ((1 < s[i-1]) && (s[i-1] < phin)) {
        if (dvalue == 1) {
            return s[i-1];
        }
    }
    return 0;
}
/*
    calculates e and d RandKitbit(), gcd() and generateD()
    arguments: uint32_t phi, totient of n
                uint32_t& mye, referenced by main(),arduinos of public key
    returns: uint32_t d, the private key of the arduino
*/
uint32_t calculateED(uint32_t phi, uint32_t& mye) {
    uint32_t d;
    bool no_d_found = true;
    while (no_d_found) {
        mye = RandKbit(15);
        while (gcd(mye, phi)!= 1 || mye > phi) {
            if (mye >= phi) {
                mye = 2;
            } else {
                mye++;
            }
        }
        d = generateD(mye, phi);
        if (d != 0) {
            no_d_found = false;
        }
    }
    return d;
}
/*
    using states from a emu, HandShakeStates, it will exchange e,n with the
        the other arduino.
    arguments: uint32_t n, arduino modulus
               uint32_t mye,arduino's public key
               uint32_t& e, other arduinos public key
               uint32_t& m, other arduinos modulus
    returns: none
*/
void handShake(uint32_t n, uint32_t mye, uint32_t& e, uint32_t& m) {
    HandShakeState currentState;
    char charbyte;
    if (isServer()) {
        currentState = SLISTEN;
    } else {
        currentState = CSTART;
    }

    while (currentState != DATAEXCHANGE) {
        if (currentState == CSTART) {
            Serial3.write('C');
            uint32_to_serial3(mye);
            uint32_to_serial3(n);
            currentState = CWAITACK;
        } else if (currentState == CWAITACK) {
             // 1 byte for 'C', and 8 bytes for e and m
            if (wait_on_serial3(9, 1000)) {
                charbyte = Serial3.read();
                if (charbyte == 'A') {
                    e = uint32_from_serial3();
                    m = uint32_from_serial3();
                    Serial3.write('A');
                    currentState = DATAEXCHANGE;
                } else {
                    currentState = CSTART;
                }
            } else {
                currentState = CSTART;
            }
        } else if (currentState == SLISTEN) {
            if (wait_on_serial3(1, -1)) {
                if (Serial3.read() == 'C') {
                    currentState = SWAITKEY;
                }
            }
        } else if (currentState == SWAITKEY) {
            if (wait_on_serial3(8, 1000)) {
                e = uint32_from_serial3();
                m = uint32_from_serial3();
                Serial3.write('A');
                uint32_to_serial3(mye);
                uint32_to_serial3(n);
                currentState = SWAITACK;
            } else {
                currentState = SLISTEN;
            }
        } else if (currentState == SWAITACK) {
            if (wait_on_serial3(1, 1000)) {
                charbyte = Serial3.read();
                if (charbyte == 'A') {
                    currentState = DATAEXCHANGE;
                } else if (charbyte == 'C') {
                    currentState = SWAITKEY2;
                } else {
                    // in case something other than C or A get passed
                    currentState = SLISTEN;
                }
            } else {
                currentState = SLISTEN;
            }
        } else if (currentState == SWAITKEY2) {
            if (wait_on_serial3(8, 1000)) {
                e = uint32_from_serial3();
                m = uint32_from_serial3();
                currentState = SWAITACK2;
            } else {
                currentState = SLISTEN;
            }
        } else if (currentState == SWAITACK2) {
            if (wait_on_serial3(1, 1000)) {
                charbyte = Serial3.read();
                if (charbyte == 'A') {
                    currentState = DATAEXCHANGE;
                } else if (charbyte == 'C') {
                    currentState = SWAITKEY2;
                } else {
                    // in case something other than C or A get passed
                    currentState = SLISTEN;
                }
            } else {
                currentState = SLISTEN;
            }
        }
    }
}
/*
    displays the arduinos keys and the other arduinos public key and modulus
    arguments: uint32_t n, arduino's modulus
               uint32_t mye, arduino's public key
               uint32_t d, arduinos's private key
               uint32_t e, other arduino's public key
               uint32_t m, other arduino's modulus
    returns: none
*/
void displaykeys(uint32_t n, uint32_t mye, uint32_t d, uint32_t e, uint32_t m) {
    Serial.print("my modulus ");
    Serial.println(n);
    Serial.print("my public key ");
    Serial.println(mye);
    Serial.print("my private key ");
    Serial.println(d);
    Serial.println("other arduino's");
    Serial.print("public key ");
    Serial.println(e);
    Serial.print("modulus ");
    Serial.println(m);
    Serial.println("Ready to talk");
    Serial.flush();
}

int main() {
    setup();
    uint32_t n, e, m, mye;

    // Determine our role
    if (isServer()) {
        Serial.println("Server");
    } else {
        Serial.println("Client");
    }
    // GenerateN returns phi(n)
    uint32_t d = calculateED(generateN(n), mye);
    handShake(n, mye, e, m);
    // Display keys
    displaykeys(n, mye, d, e, m);
    // Now enter the communication phase.
    communication(d, n, e, m);

    // Should never get this far (communication has an infite loop).

    return 0;
}
