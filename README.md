# Auto-DNS-proxy
This is my fourth years project. 

It is a simple DNS proxy which sits in the , middle of an organization's network, routing all requests to their destination. Has a cache for static local website pages.

## How It works
This app has a dedicated dns written in C which reads from an sqlite DB, for local records, but if not found, redirects to external DNS server. If the resource in question is in theloacl network, it is added to the DNS records for future reference.

for pages requested multiple times, with contents not changing, caching is applied, to reduce on the network traffic.