# Windows Package Manager Repository Policies

**Document version: 1.0**

**Document date: May 22, 2021**

**Effective date: May 22, 2021**

Thank you for your interest in providing a Product to the Windows Package Manager repository.

"Product" means content in whatever form including, but not limited to, apps, games, titles, and any additional content sold or offered from within a Product.  
"Submission" means [**Pull Request**](https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request)
 of manifest files and includes but is not limited to the "Product" and metadata about the "Product".

A few principles to get you started:

- Offer unique and distinct value within your Submission. Provide a compelling reason to download the Product from [Windows Package Manager repository](https://www.github.com/microsoft/winget-pkgs).
- Don’t mislead our customers about what your Submission can do, who is offering it, etc.
- Don’t attempt to cheat customers, the system or the ecosystem. There is no place in the repository for any kind of fraud, be it ratings and review manipulation, credit card fraud or other fraudulent activity.

Adhering to these policies should help you make choices that enhance your Submission’s appeal and audience.

Your Submissions are crucial to the experience of hundreds of millions of customers. We can’t wait to see what you create and are thrilled to help deliver your Submissions to the world.

If you have feedback on the policies or the Windows Package Manager, please let us know by commenting in our [GitHub issues forum](https://www.github.com/microsoft/winget-cli/issues)

## Table of Contents

**Product Policies:**

- [1.1 Distinct Function & Value; Accurate Representation](#11-distinct-function--value-accurate-representation)
- [1.2 Security](#12-security)
- [1.3 Product is Testable](#13-product-is-testable)
- [1.4 Usability](#14-usability)
- [1.5 Personal Information](#15-personal-information)
- [1.6 Capabilities](#16-capabilities)
- [1.7 Localization](#17-localization)
- [1.8 Financial Transactions](#18-financial-transactions)
- [1.9 Notifications](#19-notifications)
- [1.10 Advertising Conduct and Content](#110-advertising-conduct-and-content)

**Content Policies:**

- [2.1 General Content Requirements](#21-general-content-requirements)
- [2.2 Content Including Names, Logos, Original and Third Party](#22-content-including-names-logos-original-and-third-party)
- [2.3 Risk of Harm](#23-risk-of-harm)
- [2.4 Defamatory, Libelous, Slanderous and Threatening](#24-defamatory-libelous-slanderous-and-threatening)
- [2.5 Offensive Content](#25-offensive-content)
- [2.6 Alcohol, Tobacco, Weapons and Drugs](#26-alcohol-tobacco-weapons-and-drugs)
- [2.7 Adult Content](#27-adult-content)
- [2.8 Illegal Activity](#28-illegal-activity)
- [2.9 Excessive Profanity and Inappropriate Content](#29-excessive-profanity-and-inappropriate-content)
- [2.10 Country/Region Specific Requirements](#210-countryregion-specific-requirements)
- [2.11 Age Ratings](#211-age-ratings)
- [2.12 User Generated Content](#212-user-generated-content)

## Product Policies

### 1.1 Distinct Function & Value; Accurate Representation

The Product and its associated metadata, including but not limited to the app title, description, screenshots, trailers, content rating and Product category, must accurately and clearly reflect the source, functionality, and features of the Product.

### 1.1.1

All aspects of the Product should accurately describe the functions, features and any important limitations of the Product.

### 1.1.2

[Tags](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/manifests/v1.0.0/manifest.defaultLocale.1.0.0.json) may not exceed 16 unique tags and should be relevant to the Product.

### 1.1.3

The Product must have distinct and informative metadata and must provide a valuable and quality user experience.  

### 1.1.4
The [InstallerUrl](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/manifests/v1.0.0/manifest.defaultLocale.1.0.0.json) must be the ISVs release location for the Product.  Products from download websites will not be allowed.  

### 1.2 Security

The Product must not jeopardize or compromise user security, or the security or functionality of the device, system or related systems.

### 1.2.1

The Product must not attempt to change or extend its described functionality through any form of dynamic inclusion of code that is in violation of Windows Package Manager Policies. The Product should not, for example, download a remote script and subsequently execute that script in a manner that is not consistent with the described functionality.

### 1.2.2

The Product must not contain or enable malware as defined by the Microsoft criteria for [Unwanted and Malicious Software](/windows/security/threat-protection/intelligence/criteria).

### 1.2.3

The Product may contain fully integrated middleware (such as third-party cross-platform engines and third-party analytics services).

The Product may depend on non-integrated software (such as another Product, module, or service) to deliver its primary functionality, subject to the following requirements:

### 1.3 Product is Testable

The Product must be testable. If it is not possible to test your submitted Product for any reason your Product may fail this requirement.

### 1.4 Usability

The Product should meet usability standards, including, but not limited to, those listed in the subsections below.

### 1.4.1

The Product should support the devices and platforms on which they are downloaded, including compatibility with the software, hardware and screen resolution requirements specified by the Product. If the Product is downloaded on a device with which it is not compatible, it should detect that at launch and display a message to the customer detailing the requirements.

### 1.4.2

The Product should continue to run and remain responsive to user input. Products should shut down gracefully and not close unexpectedly. The Product should handle exceptions raised by any of the managed or native system APIs and remain responsive to user input after the exception is handled.

### 1.4.3

The Product should start up promptly and must stay responsive to user input.

### 1.5 Personal Information

The following requirements apply to Products that access Personal Information. Personal Information includes all information or data that identifies or could be used to identify a person, or that is associated with such information or data.

### 1.5.1

If the Product accesses, collects or transmits Personal Information, or if otherwise required by law, it should maintain a privacy policy. The submission, should include the [PrivacyUrl](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/manifests/v1.0.0/manifest.defaultLocale.1.0.0.json) which links to the privacy policy of the Product.

### 1.5.2

If the Product publishes the Personal Information of customers of the Product to an outside service or third party, the Product should only do so after obtaining opt-in consent from those customers. Opt-in consent means the customer gives their express permission in the Product user interface for the requested activity, after the Product has:

- described to the customer how the information will be accessed, used or shared, indicating the types of parties to whom it is disclosed, and
- provided the customer a mechanism in the Product user interface through which they can later rescind this permission and opt-out.

### 1.5.3

If the Product publishes a person’s Personal Information to an outside service or third party through the Product or its metadata, but the person whose information is being shared is not a customer of the Product, the Product must obtain express written consent to publish that Personal Information, and must permit the person whose information is shared to withdraw that consent at any time. If the Product provides a customer with access to another person’s Personal Information, this requirement would also apply.

### 1.5.4

If the Product collects, stores or transmits Personal Information, it must do so securely, by using modern cryptography methods.

### 1.5.5

The Product must not collect, store or transmit highly sensitive personal information, such as health or financial data, unless the information is related to the Product’s functionality. The Product must also obtain express user consent before collecting, storing or transmitting such information. The Product’s privacy policy must clearly tell the user when and why it is collecting Personal Information and how it will be used.

### 1.5.6

If the Product supports Microsoft identity authentication it must do so only by using Microsoft-approved methods.

### 1.5.7

Products that receive device location must provide settings that allow the user to enable and disable the Product's access to and use of location from the Location Service API.

### 1.6 Capabilities

If the Product declares the use of capabilities, then the capabilities the Product declares must legitimately relate to the functions of the Product.  The Product must not circumvent operating system checks for capability usage.

### 1.7 Localization

If the Product you should provide localized all languages that it supports. The experience provided by a product must be reasonably similar in all languages that it supports.

### 1.8 Financial Transactions

If your product includes in-product purchase, subscriptions, virtual currency, billing functionality or captures financial information, the following requirements apply:

### 1.8.1

In-product offerings sold in your product cannot be converted to any legally valid currency (for example, USD, Euro, etc.) or any physical goods or services.

### 1.8.2

The Product must use a secure purchase API for purchases of physical goods or services, and a secure purchase API for payments made in connection with real world gambling or charitable contributions. If the Product is used to facilitate or collect charitable contributions or to conduct a promotional sweepstakes or contest, it must do so in compliance with applicable law. The Product must also state clearly that Microsoft is not the fundraiser or sponsor of the promotion.

The Product must use a secure purchase API to receive voluntary donations from users.

The following requirements apply to your use of a secure purchase API:

- At the time of the transaction or when the Product collects any payment or financial information from the customer, the Product must identify the commerce transaction provider, authenticate the user, and obtain user confirmation for the transaction.
- The product can offer the user the ability to save this authentication, but the user must have the ability to either require an authentication on every transaction or to turn off in-product transactions.
- If the product collects credit card information or uses a third-party payment processor that collects credit card information, the payment processing must meet the current PCI Data Security Standard (PCI DSS).

### 1.8.3

The product and its associated metadata must provide information about the types of in-product purchases offered and the range of prices. The Product not mislead customers and must be clear about the nature of the in-product promotions and offerings including the scope and terms of any trial experiences. If the Product restricts access to user-created content during or after a trial, it must notify users in advance. In addition, the Product must make it clear to users that they are initiating a purchase option in the Product.

If your game offers “loot boxes” or other mechanisms that provide randomized virtual items, then you must disclose the odds of receiving each item to customers prior to purchase. These disclosures may appear: in-product, such as in an in-app store, on the Microsoft Store Product Description Page (PDP), and/or on a developer or publisher website, with a link from the Store Product Description Page (PDP) and/or in-app.

### 10.8.4

All pricing, including sales or discounting, for your digital products or services shall comply with all applicable laws, regulations and regulatory guidelines, including without limitation, the Federal Trade Commission [Guides Against Deceptive Pricing](https://www.ecfr.gov/cgi-bin/text-idx?SID=676bd39fe43a808fcb417973b3d0247e&mc=true&tpl=/ecfrbrowse/Title16/16cfr233_main_02.tpl).

### 1.9 Notifications

If the Product supports notifications, then the Product must respect system settings for notifications and remain functional when they are disabled. This includes the presentation of ads and notifications to the customer, which must also be consistent with the customer’s preferences, whether the notifications are provided by the Microsoft Push Notification Service (MPNS), Windows Push Notification Service (WNS) or any other service. If the customer disables notifications, either on an Product-specific or system-wide basis, the Product must remain functional.

### 1.10 Advertising Conduct and Content

For all advertising related activities, the following requirements apply:

### 1.10.1

- The primary purpose of the Product should not be to get users to click ads.
- The Product may not do anything that interferes with or diminishes the visibility, value, or quality of any ads it displays.
- The Product must respect advertising ID settings that the user has selected.
- All advertising must be truthful, non-misleading and comply with all applicable laws, regulations, and regulatory guidelines.

## Content Policies

The following policies apply to content and metadata (including publisher name, Product name, Product icon, Product description, Product screenshots, Product trailers and trailer thumbnails, and any other Product metadata) offered for distribution in the Windows Package Manager repository. Content means the Product name, publisher name, Product icon, Product description, the images, sounds, videos and text contained in the Product, the tiles, notifications, error messages or ads exposed through the Product, and anything that’s delivered from a server or that the Product connects to. Because Product and the Windows Package Manager repository are used around the world, these requirements will be interpreted and applied in the context of regional and cultural norms.

### 2.1 General Content Requirements

Metadata and other content you submit to accompany your submission may contain only content that would merit a rating of PEGI 12, ESRB EVERYONE 10+, or lower.

### 2.2 Content Including Names, Logos, Original and Third Party

All content in the Product and associated metadata must be either originally created by the application provider, appropriately licensed from the third-party rights holder, used as permitted by the rights holder, or used as otherwise permitted by law.

### 2.3 Risk of Harm

### 2.3.1

The Product must not contain any content that facilitates or glamorizes the following real world activities: (a) extreme or gratuitous violence; (b) human rights violations; (c) the creation of illegal weapons; or (d) the use of weapons against a person, animal, or real or personal property.

### 2.3.2

The Product must not: (a) pose a safety risk to, nor result in discomfort, injury or any other harm to end users or to any other person or animal; or (b) pose a risk of or result in damage to real or personal property. You are solely responsible for all Product safety testing, certificate acquisition, and  implementation of any appropriate feature safeguards. You will not disable any platform safety or comfort features, and you must include all legally required and industry-standard warnings, notices, and disclaimers in the Product.

### 2.4 Defamatory, Libelous, Slanderous and Threatening

The Product must not contain any content that is defamatory, libelous, slanderous, or threatening.

### 2.5 Offensive Content

The Product and associated metadata must not contain potentially sensitive or offensive content. Content may be considered sensitive or offensive in certain countries/regions because of local laws or cultural norms. In addition, the Product and associated metadata must not contain content that advocates discrimination, hatred, or violence based on considerations of race, ethnicity, national origin, language, gender, age, disability, religion, sexual orientation, status as a veteran, or membership in any other social group.

### 2.6 Alcohol, Tobacco, Weapons and Drugs

The Product must not contain any content that facilitates or glamorizes excessive or irresponsible use of alcohol or tobacco Products, drugs, or weapons.

### 2.7 Adult Content

The Product must not contain or display content that a reasonable person would consider pornographic or sexually explicit.

### 2.8 Illegal Activity

The Product must not contain content or functionality that encourages, facilitates or glamorizes illegal activity in the real world.

### 2.9 Excessive Profanity and Inappropriate Content

- The Product must not contain excessive or gratuitous profanity.
- The Product must not contain or display content that a reasonable person would consider to be obscene.

### 2.10 Country/Region Specific Requirements

Content that is offensive in any country/region to which the Product is targeted is not allowed. Content may be considered offensive in certain countries/regions because of local laws or cultural norms. Examples of potentially offensive content in certain countries/regions include the following:

China

- Prohibited sexual content
- Disputed territory or region references
- Providing or enabling access to content or services that are illegal under applicable local law

### 2.11 Age Ratings

The Product should have a age rating that would merit a rating of PEGI 12, ESRB EVERYONE 10+, or lower.

### 2.11.1

If the Product provides content (such as user-generated, retail or other web-based content) that might be appropriate for a higher age rating than its assigned rating, you must enable users to opt in to receiving such content by using a content filter or by signing in with a pre-existing account.

### 2.12 User Generated Content

User Generated Content (UGC) is content that users contribute to an app or Product and which can be viewed or accessed by other users in an online state. If the Product contains UGC, the Product should:

- Publish and make available to users a Product terms of service and/or content guidelines for User Generated Content either in Product or on the Product website.
- Provide a means for users to report inappropriate content within the Product to the developer for review and removal/disablement if in violation of content guidelines and/or implement a method for proactive detection of inappropriate or harmful UGC.
- Remove or disable UGC when requested by Microsoft.

### See also

- [Change history for Windows Package Manager Policy History](.\windows-package-manager-policies-change-history.md)
- [Windows Package Manager Code of Conduct](https://github.com/microsoft/winget-pkgs/blob/master/CODE_OF_CONDUCT.md)
- [Windows Package Manager Contributing requirements](https://github.com/microsoft/winget-pkgs/blob/master/README.md)
 