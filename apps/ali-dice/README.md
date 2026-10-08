# ali-dice 1.2.9 - Terry's Dice, the ALI security base

The house recipe (free-RAM x used-disk + free-RAM, uptime, boot id,
load) hashed with 32 FRESH kernel-CSPRNG bytes per roll. Stats make it
yours, kernel bytes make it unguessable. Fails closed.

- `terry-dice hex [bytes]` - hex roll (default 32 bytes / 64 chars)
- `terry-dice id` - stable machine-unique install ID (first roll kept)
- `ali-passgen [len]` - 24-char (96-bit) random password
- `ali-passgen --words [n]` - 6 memorable words (~42 bits, logins only)
- `ali-vault lock|unlock|list` - AES-256-CBC file vault, dice-rolled
  data key wrapped by your password (PBKDF2). Wrong password = ALERT
  in the Sentinel feed, so vault brute-force escalates to the IPS.

Threat model: stops remote attackers and thieves. Not root malware
(nothing software-only does). Vault needs `openssl` (in the image).
