---
id: XDN00
title: The Design Note Process​
author: hpierson@microsoft.com
status: draft
---

# XLDN00 - The Design Note Process​

- Harry Pierson (hpierson@microsoft.com)
- Status: Draft

## Abstract

This document describes the process for making and disseminating architecturally important design decisions through design notes in the [xlang project](http://github.com/Microsoft/xlang).

Thanks to [Galen Hunt](https://www.microsoft.com/en-us/research/people/galenh/) for pioneering the use of the design note process and allowing us to use it on this project.

## 1\. The Purpose

In any project, the process for making and documenting decisions defines the character of the endeavor. The xlang project is small now, but is likely to grow and become a collaborative effort across multiple divisions of Microsoft as well as the open source community.

To foster collaboration, the decision closure process used within the project should have four qualities; it should be:

- **Transparent**. Anyone with interest should understand how decisions are made and what mechanisms are available to them to affect decision making.

- **Scalable**. The process should allow consistent decisions at the individual and the project level. The process should allow participants to scale their involvement based on relevance to their role. The process should allow input from all project participants.

- **Lightweight**. The time and effort required to produce a good decision should be small. Likewise, the time and effort to update a decision in light of changing inputs should also be small.

- **Discoverable**. An outsider or someone new to the effort should be able to quickly determine the set of completed decisions and the motivation for those decisions. New project members should understand and efficiently participate in the process with little training.

Design notes are _not_ a substitute for documentation. Project documentation describes _how_ the system works. Design notes capture _why_ the team chose to build the system the way that they did.

## 2\. The Process

The design note process provides a mechanism for making collective decisions using design notes as artifacts to describe decisions and design note reviews as a mechanism for evaluating, building consensus, and committing a decision to the project's plan of record. A design note is authored by a working group a group of volunteers interested in the topic of the design note.

The design note process is directed toward producing progress collaboratively in an area of high risk and uncertainty. An individual design note is created to reduce the space of the unknown and remove uncertainty from the project.

Participants in the design note process include:

- The **_Community_**: everyone using or otherwise interested in the development of the project.

- The **_Design Note Author_**: person who volunteers to write a design note on a particular topic of interest.

- The **_Design Note Working Group_**: the author and other persons who volunteer to participate in discussions and efforts contributing to the design note. Members of the working group are also known as contributors to reflect their active involvement in the authoring process.

- The **_Design Note Moderator_**: person who oversees the process of vetting design notes for formal acceptance as part of the plan of record. The moderator schedules and announces formal design note reviews, moderates discussions during the reviews to ensure that all viewpoints are expressed, judges consensus in reviews, and publishes the outcomes of formal design note reviews.

> Note: Currently, a design note moderator has not been formally chosen for the xlang project. For now please contact [Ben Kuhn](mailto:benkuhn@microsoft.com) and [Harry Pierson](mailto:hpierson@microsoft.com).

- The **_Design Note Review Committee_**: a small body of representatives chosen from the larger community. Members of the review committee are chosen to represent all of the major engineering groups participating in the project. As a body, the review committee has the final say of whether or not a design note becomes an accepted part of the plan of record.

> Note: Currently, the xlang project review committee consists entirely of Microsoft employees. After an initial project bootstrap period, we will open up this committee to include representatives from the larger xlang community.

Design notes are used to communicate design decisions and information that should be shared with a wide audience of contributors to the project. The design note process is also used to gather input when complex or potential controversial decisions must be made. In general, narrowly scoped issues with few interested parties will not be documented through the design note process. For example much of the content of an implementation specification or a usage guide would not be appropriate for a design note. On the other hand, a decision such how to support UTF-8 strings should be included in a design note as it 1) has wide-ranging implications, and 2) would benefit from the wisdom of the crowd.

The steps of the design note process are as follows:

1. **An individual announces the desire to produce a design note (and form a working group) on a specific topic**. This self-chosen author can make an informal announcement at a work stream meeting or via an email to an appropriate mailing list (to gauge interest). A best practice is to send an announcement email with a description of the question the working group intends to address. A design note author should attempt to identify and include all relevant subject-matter experts.

2. **Individuals volunteer to participate in the working group by sending email to the design note author**. The cost of admission to a working group is a commitment to actively contribute to progress and to reaching consensus. Conversely, by abstaining from a working group, community members implicitly give the working group their permission to draft a design note as they see fit.

3. **The design note author leads discussions to draft a design note**. These discussions will often gather feedback from the community via email and refine ideas through small group meetings filtered to committed contributors.

4. **The working group writes a design note using the design note template**. The design note template is available in the design_notes folder of the project's git repo. Design note numbers are assigned sequentially in order of request by placing the first draft of the design note in the git repo (either directly or through the moderator) using the next available number. Design note drafts are marked with "Draft" status to encourage interested parties to read them. The working group may circulate early drafts of the design note via email or other collaboration tools to help interested parties track the design note evolution.

5. **A formal design note review is scheduled through the moderator when the working group has completed its draft**. The moderator selects a date and schedules a review meeting with the review committee. The moderate announces the schedule for the review via an email to the appropriate mailing lists.

6. **The community provides feedback on the design note to the authors, to members of the review committee, or as a last resort to the moderator**. Authors are expected to give appropriate consideration to feedback from the community. Some working groups will use informal design note reviews to gather and process community feedback. Note however that these informal reviews with volunteers from the community do not take the place of formal reviews with the review committee.

7. **The formal design note review is used to determine if the design note should be adopted by the project**. The purpose of the design review is to summarize the decisions and data incorporated into the design note, to provide feedback to the working group from the primary interest of the project community, and ultimately to determine if the design note should be adopted.

   1) **Attendees should prepare for the review by reading the design note and prioritizing their comments**. Attendees who fail to read the design note before the meeting often miss the opportunity to understand why specific decisions have been made. Because participants are prepared, design reviews can be brief (never to exceed 90 minutes).

   2) **The moderator calls the meeting to order**.

   3) **The design note author leads the discussion of the design note**. As attendees have read the design note, most of the discussion focuses on points of clarification and concern. Because design notes generally start with principles and priorities listed first followed by interface then implementation, the discussion flows through the design note from beginning to end. The leader will typically summarizing each section, and then ask for comments. A high level of open discussion is encouraged during reviews.

   4) **The moderator ensures that all participants have an opportunity to speak, that issues are addressed, and that a balanced discussion occurs**. To encourage equity and the broadest input, the moderator may suggest that well-worn discussions be taken offline and proactively call on individuals to voice their viewpoints. The moderator makes sure that areas of consensus and concern are noted and addressed.

   5) **The moderator calls for a vote on the design note at the end of the meeting**. The review committee may decide to approve a design note, approve it subject to edits, or return the design note to the working group for further work. Strictly speaking, a majority vote is sufficient for approval of a design note; however, in practice, the design note review process is intended to produce a widely accepted consensus decision.

8. **After acceptance, the final design note is published**. The design note is archived in the design notes folder of the project repo with its status changed to "Adopted". The moderator sends email with the design note's URL to the appropriate mailing lists announcing that the design note is complete and adopted. If the design note replaces or invalidates earlier adopted design notes, their status is changed to "Deprecated".

## 3\. The Decisions

The design note process is most effective when individual participants recognize that design notes are the project's primary process for decision making and learning.

To you as an individual, the design note process gives well defined points at which you can influence the evolution and architecture of the project. You can choose which decision points to exercise based on the relevance of a particular issue to your personal and professional interests and agenda. In summary, the decision points available to you are as follows:

- **Formation of a working group**. You can propose the formation of a new working group. You can also vote with your feet to join or abstain from a working group.
- **Working group discussions**. You can contribute to working group discussions. You should especially do so if the topic is an area of importance to you.
- **Design note authoring and editing**. You can contribute content to the design note.
- **Design note review**. You can add suggestions and raise concerns to the author(s) and through the review committee and the moderator for discuss at the formal design note review.
- **Follow-up**. You create a new design note that expands on or even replaces a previous note.

Because all are given equal access to affect the outcome of the design note process, **it is expected that you will in good faith support the decisions incorporated in the adopted design notes**.

Participants often overlook the iterative nature of the design note process. Any adopted design note can be updated, augmented, replaced, or deprecated using the design note process.

To maximize your input while minimizing your meeting burden, a good rule of thumb is to read design notes and provide feedback for the design note reviews, but to join only working groups where you intend to be put your engineering energy.

The design note process is not a funding process. The committee vets design decisions, but doesn't provide funding for their implementation.

## 4\. The Content

An individual design note has three roles:

1. The design note **drives understanding** of a topic
2. The design note **drives a set of decisions** on a particular topic
3. The design note **provides documentation of the decision** to allow later readers to understand both the outcome and motivation for the decision.

Ideally design notes are concise, clear, and definitive. A design note should be short enough to be read in a single sitting; 3 to 6 pages of body is a good rule of thumb. A design note should be clear enough that it can be read by a general audience (or at least as general an audience as needs to be informed of the decisions discussed). Finally, a design note should clearly define a set of decisions.

A design note should have sufficient clarity and content to stand on its own. As a rule of thumb, a new employee should be able to read a design note and understand what decisions were made and why without requiring extra material. Where a design note builds on earlier decisions, it should explicitly reference the appropriate previous design notes.

As stated earlier, _design notes are not a substitute for project documentation_. A given area of the project may have multiple decisions that need to be made - and thus multiple design notes. For example, there are multiple decisions that need to be made for the xlang platform adaptation layer regarding areas such as string encoding, error origination APIs and activation registration. Each of these decisions should have a separate design note, timeline, author and working group. The details captured in these design notes will inform future team members who wish to understand _why_ a specific area of the platform adaptation layer works the way that it does. However, there needs to be separate documentation of the platform adaptation layer itself that enables future team members to use or port the platform adaptation layer without needing to review all of the details that went into the decision making process.

The _most successful design notes_ (and therefore the most widely referenced) clearly separate objectives, priorities, principles, interfaces, abstractions and implementations. A given design note may target all or only a few of these points. Particularly, in areas of active exploration and learning, a design note will target a specific project phase. For example, an exploratory design note might state the problem and propose a method for evaluating potential solutions based on a set of priorities and principles. Later design notes might abstract the interface to a solution. Subsequent design notes might describe the core abstractions of an implementation and finally describe the implementation details of a system. In practice, putting forth the guiding principles and priorities is often sufficient for collaboration.

Experience has shown that design notes for a particular topic have a natural four-phase progression: 1) Principles and Axioms, 2) Architecture and Abstractions, 3) Implementation, 4) Usage. For example, the first design note for a networking stack will outline the objectives and design axioms, "our networking stack will provide the simples TCP/IP implementation in order provide compatibility and create a code base that is easily understood by any intern." The second design note will describe the key abstractions, "our networking stack will be layered with no back-door interfaces between MAC, IP and TCP." The third design note will describe the implementation, "our TCP layer is divided into four classes..., etc." The fourth design note will describe how to use the code, "to install our TCP/IP stack on your system..., etc."

All four phases of design notes are _not_ needed for all projects. In general if many people need to know a detail because it impacts their work, then it probably needs a design note. Quite often, a project will only have principles-and-axioms and architecture-and-abstractions design notes as the reader can consult the straight-forward code for implementation details because these are the only details needed by anyone other than the implementer. As another example, an interface compiler might be documented with a principles-and-axioms design note and a usage design note because the compiler is used by many people, but altered only by a few programmers.

The _least effective design notes_ are those that are written primarily as post-mortem documentation or without encouraging collaboration by interested parties in the writing process. These design notes fail to help participants explore and discuss design trade-offs. As apologists for the code or a personal agenda, they tend to obscure the decision process rather than make it transparent. They don't provide principles that can be used to make later decision about lower level details or other parts of the project; they often don't even help inform design on replacement.

The _least effective working groups_ fail because they attempt to overload too much into a single design note or they confuse writing design notes with writing documentation. Overloading produces two problems: writing the design note becomes onerous and decision making becomes insular. When a design group goes dark, casual (and often insightful) participants have little room for input.

Design notes should be based upon a standard design note Template. This document uses one such template.
