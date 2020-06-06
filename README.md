# SHA256-ADS
Hashes a file while including it's alternate data streams (ADS) (using windows API to retrieve streams).

Naively concatenates the hashes of all the streams and then hashes the concatenation, which can identify any changes in ADS as well.
