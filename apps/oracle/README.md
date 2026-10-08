# oracle 1.3.2 - ALI security voice

One daemon, one feed (`/run/ali-oracle`), two voices:

- `[verdict]` - security state rendered from the Sentinel feed
  (blocks/alerts/vault fails), Odysseus warnings + sea state. Speaks
  only when the state changes - Center's Security tab reads this.
- `[lots]` - the templeos word-oracle, relayed (templeos still owns
  generation: words, verses, retro mode).

Raw feeds stay as sources; Oracle is the voice. `oracle --once`
renders one pass now.
