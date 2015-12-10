# partial-keysearch
AES partial key search in C


The Advanced Encryption Standard (AES) is a specification for the encryption for digital information. 
Essentially, this process starts by feeding the plain-text massage into the AES encryption function. This
function uses a 256-bit key (which is normally generated using a randomised generation process) to
produce a stream of cipher-text, that contains the original message in an encrypted form. The encrypted
message can then be transmitted through an un-secure channel, to a destination where the AES decrypt
function can be used to recover the plain-text from the cipher-text. The AES decrypt function uses the same
256-bit key, that wae used for the original encryption process, to convert the cipher-text back to the plain-
text.
Clearly, the security of this system is reliant on the key that is used to encrypt and decrypt the plain text
remaining secret. If a potential attacker is able to get access to a sample of plain-text and its corresponding
cipher-text, an exhaustive key search can be used to determine the key used to produce cipher text from
the given plain text. This basically involves trying every possible value for the 256-bit key on the sample of
cipher-text until a value is found that produces the corresponding cipher-text. The problem with this
approach is that it will require (worst-case) 2^256 encryption operations. The universe will end long before
the key is every found. If (through coercion, espionage or a phishing attack) you are able to obtain part of
the 256-bit key, the search space can be drastically cut-down resulting in a more achievable partial key
search.
In this assignment you will write a multi-process program to carry out a partial key search. The input forthis program will be a stream of cipher-text, a stream of corresponding plain-text and 236 bits of the 256 key
that was used for the original encryption.
Your program will use a ring of processes to search for the full encryption key used to generate a piece of
cipher-text.

You will implement a multiprocess version of this key-search program that uses pipes arranged using the
ring topology (i.e. a ring of processes) for inter-process communication. Your program should take thenumber of processes and the partial key as command line arguments and read the supplied plain-text and
cipher-text from the file plain.txt and cipher.txt. Your program will need to divide the search space up
amongst the processes and execute a partial search in each process.

Within the ring topology, you will need to develop a system/protocol that communicates the full key (from
the process that discovers it) around the ring and back to the parent node. How this is actually done is up to
you.
