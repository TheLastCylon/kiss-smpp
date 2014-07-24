Welcome!

Before reading any further, please take note:

KISS-SMPP is *not*, I repeat, *not* a library that will assist you with creating
your own SMPP client.

The intent behind KISS-SMPP, is to completely negate your need to create a
custom SMPP client and/or server. As such, the name "KISS-SMPP" does not refer
to a single application, but rather an applications-suite and several libraries,
that endevor to do the SMPP specific stuff for you.

The entire SMPP Session Management process is tucked away, where you'll never
have to touch it if you don't want to. All that's expected from you, is to know
how to use basic networking (*in the technology of your choice*) and have some
mechanism for decoding and encoding data from and to *JSON* format.

If you have that, you are almost ready to start using the KISS-SMPP client, for
sending and recieving text messages.

Take a look at the interface specification of the KISS-SMPP Client (ksmppcd).
You'll find that you'll never have to issue a Bind request or deal with
enquire\_link PDUs. Once you've completed the configuration of the client, you
could manage the sending and recieving of messages with nothing more than a well
written shell script. *It might be foolish, but it would be possible.*

# Terms and Definitions

| Term | Definition |
|------|------------|
| KISS | **K**eep **I**t **S**imple **S**tupid |
| SMPP | **S**hort **M**essage **P**eer-to-peer **P**rotocol |
| MC   | **M**essage **C**entre (the people you connect to, using an smpp client) |
| ESMC | **E**xternal **S**hort **M**essage **C**entre (The entity connecting to a Message Centre) |

# Basics of interfacing with KISS-SMPP Client (ksmppcd)

All applications in the KISS-SMPP application suit, use JSON as an interface
protocol. There are many reasons for this, but here are some major ones:

- It's human readable.
- It's much less bandwidth intensive than XML, SOAP, etc., etc., etc.
- It's incredibly well supported across major programming/scripting languages.




TODO: Define message centre.

|   |   |   |   |
|---|:-:|---|---|
| Parameter        | Mandatory? | Valid Values                 | Clarification     |
| cmd              |   Yes      | send                         |  The send command |
| source-addr      |   Yes      | a valid address for your MC. |                   |
| destination-addr |   Yes      | a valid address for your MC. |                   |
| short-message    |   Yes      | The text message you want to send ||

socket and your applications are expected to expose a few callback interfaces.
Other than that, the aim is to be as platform and technology agnostic, as can be reasonably expected.


You will not be forced to write code in any language
you feel uncomfortable with. Nor will you need to cram new and foreign ideas into your organization.

1.  *a collection of applications and libraries* that endevour to completeley negate the need for having to create your own **SMPP** client.



You are reading this so chances are good, that you want to know what **KISS-SMPP** is.
Let's get the obvious out of the way first:
KISS is an acronym for **K**eep **I**t **S**imple **S**tupid, and SMPP is an acronym for **S**hort **M**essage **P**eer-to-peer **P**rotocol.
From those two acronyms, you'd possibly be led to think, that **KISS-SMPP** is a library aimed at simplifying your life, with regard to implementing your own **SMPP** client.
However, you'd be mistaken.

So, what is it then?

Well, first off, **KISS-SMPP** isn't a library, it's a collection of applications and libraries 


~~~~
A code block
this is a cool code block
~~~~

## This is a test

### This is a test

1. list item
    - asdf
    - asdf
    - asdf
      - qwer
      - qwer
      - qwer
    - asdf

~~~~
A code block
~~~~

asdf

1. list item
1. list item
1. list item
1. list item
1. list item
1. list item
1. list item
1. list item

