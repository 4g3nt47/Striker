# Usage Guide

After following the instructions in the project [README](/README.md), you should have *Striker* up and running. This guide walks you through the web UI.

## Setup

For this demo, I'm using the following setup;

1. Frontend / Web UI: This is hosted at `https://striker.debian.local`, which is an Nginx reverse proxy that forwards to a HTTP server hosting the web files at `http://localhost:8000`;
```text
server {
    listen 443 ssl;
    server_name             striker.debian.local;
    ssl_certificate         /etc/ssl/certs/wildcard.debian.local.pem;
    ssl_certificate_key     /etc/ssl/private/wildcard.debian.local.key;
    access_log              /var/log/nginx/striker.debian.local;

    location / {
      proxy_pass              http://127.0.0.1:8000;
      proxy_redirect          off;
      proxy_read_timeout      90;
      proxy_set_header        Host $host;
      proxy_set_header        X-Real-IP $remote_addr;
      proxy_set_header        X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```
2. Redirector: A dumb pipe redirector at `172.17.0.1:4443` that forwards to the C2 server running locally on the same host.
3. C2 server: Running on `https://localhost:3000`.
4. Target host: A docker container at `172.17.0.2` running Debian 10 (Buster).
5. Agent: A C agent built for linux, with SSL verification disabled since I'm using a dumb pipe redirector.
6. Operators: `admin` (an admin account), and `guest` (a user account).

## The Web UI

**1) Agents Menu**

This is where you will find yourself following a successful login;

![Pasted image 20221022093325](https://user-images.githubusercontent.com/54174043/199900299-a690e022-b735-4bf4-8368-a1f67878a7bf.png)

This is where all the agents in the server, active ones or otherwise, will be listed. To interact with an agent, simply click on it in the list.  This will open the agent handling menu, which has **4** tabs;

The **Info** tab shows some information about about the agent, including the agent ID, the operating system, the name of the user the agent is running as, and more;

![Pasted image 20221022093659](https://user-images.githubusercontent.com/54174043/199900377-c4dc222a-41ce-4482-bf1f-33c4565d7c25.png)

The **Tasks** tab shows a list of all tasks created for the agent;

![Pasted image 20221022094156](https://user-images.githubusercontent.com/54174043/199900464-b8e899f6-82cb-4311-8408-b17b0e6f80b3.png)

Like most lists you will see in the UI, the listed tasks are clickable, and doing so will open a modal with information about the agent;

![Pasted image 20221022094434](https://user-images.githubusercontent.com/54174043/199900593-2da4d9c8-a135-4bb0-8c32-ffc086d99391.png)

Note that although the scrollbar is invisible in the modal, you can still scroll down when some text are out of view. The task result field also has a scroll feature of it's own, which helps for tasks with large result.

(Tip: hovering on a listed task will show a small tooltip with the task data);

The contents of the task modal will vary depending on the state of the task. For tasks that have been received by the agent, but not yet completed, you will have an option to kill the task. As this is merely a flag passed to the running task by the agent, it won't work for some tasks;

![Pasted image 20221022094746](https://user-images.githubusercontent.com/54174043/199900689-c02a5997-93d8-4138-aa8d-665a0cf466f2.png)

The **Console** tab provides you with a simple console for interacting with the agent. This is where you will likely spend most of your time when handling active agents. To get a list of all available commands in the context of the agent, type `help` or `?`;

![Pasted image 20221022194149](https://user-images.githubusercontent.com/54174043/199900964-0241c200-c754-484d-870a-a54a9e691614.png)

The following is an example of using the console to run a shell command on the agent;

![Pasted image 20221022194310](https://user-images.githubusercontent.com/54174043/199901056-7bd1d536-7c27-4e98-a520-47432215ef33.png)

(Tip: You can use the up and down arrow key to move between previous commands, which could save you from retyping long commands)

The **Files** tab is where you upload files to the agent, or download them to your machine. When uploading a file to an agent, the file is first uploaded to the C2 server, and a task is then automatically created for the agent to download the uploaded file. The file will be saved to the working directory of the agent, so make sure you are in the right place. A successful upload for a file looks something like this;

![Pasted image 20221022194646](https://user-images.githubusercontent.com/54174043/199901217-ae38e64d-3ee2-433e-bd6d-2f32039861d1.png)

Going to the agent console tab, you should see something like this (the text in green is from the file upload to by the user, and the automatic download by the agent);

![Pasted image 20221022194848](https://user-images.githubusercontent.com/54174043/199901298-d8ea6a5c-59a9-4a11-9678-1f4476c15c81.png)

Files you download from an agent are also displayed in the **Files** tab. Clicking on a listed file will open a modal for the file;

![Pasted image 20221022195240](https://user-images.githubusercontent.com/54174043/199901424-fd08c220-15d2-4f0a-afd0-94125c65e177.png)

**2) Redirectors Menu**

This menu is used to maintain a list of URLs for active redirectors. This list is sent to all agents when they first connect to the server.

![Pasted image 20221022103124](https://user-images.githubusercontent.com/54174043/199901560-e76f3f8d-9dc1-4e0b-91cc-2ac9ad42082f.png)

Note that this must not necessarily be a redirector, but can also be another C2 server. This is because for an agent, there is no difference between the two. Just make sure that all redirectors and C2 servers in the list lead to C2 servers that share the same backend database. This is because should a redirector or a server go down while an agent has an active session on it, the agent will fallback to other redirectors/servers and attempt to resume the session it was previously using. Should it fail, the agent will exit.

**3) Auth Keys Menu**

This menu is used to manage the authentication keys used by agents when creating a session on the server. There are two types of keys;
1. `volatile` - This is a key that can only be used once. Once used, it will be automatically deleted by the server. This is the recommended key type to use since someone may run your agent multiple times to flood you with sessions. A downside to this is you will need to build a new agent with a new auth key for every session.
2. `static` - This key can be used multiple times. Saves you from having to build multiple agents with different keys each.

![Pasted image 20221022104934](https://user-images.githubusercontent.com/54174043/199901825-7dc5b52b-a3dd-4f74-80ab-d2c1dd75e069.png)

To add a new key, click on the **Add Key** button. This will open a modal for adding a new key with a random string;

![Pasted image 20221022105543](https://user-images.githubusercontent.com/54174043/199902041-32832aeb-524a-43ad-bf29-77f07e4d7d7c.png)

Clicking on a listed key opens a modal;

![Pasted image 20221022105757](https://user-images.githubusercontent.com/54174043/199902259-3e2a9ec2-5b64-470e-b352-1a3b73d749ca.png)

**4) Team Chat Menu**

This menu is used for communicating with other operators over plain text. Some server events are also displayed here, like user login and new agent callbacks;

![Pasted image 20221022195612](https://user-images.githubusercontent.com/54174043/199902420-c29383ac-5edf-4b52-b215-67951ac1f0e8.png)

Inputs that start with `/` are treated as commands;
- `/users` - List logged in users.
- `/clear` - Clear chat logs, in case someone committed some cringe :)

**5) Users Menu**

For admins only. You will be using this to manage all operator accounts on the server;

![Pasted image 20221022111716](https://user-images.githubusercontent.com/54174043/199902593-4b43aa62-f8b2-422f-8e0a-c6a4f687d63c.png)

Notice the color difference in the listed users. Online users are colored green, suspended/disabled account are colored cyan, and others are colored black.

Clicking a user will open a modal you can use to reset account password, manage admin privileges, suspend/activate the account, or delete the account;

![Pasted image 20221022112113](https://user-images.githubusercontent.com/54174043/199902681-75543d47-27f5-481d-98c4-c4b66264df27.png)

**6) Event Logs Menu**

For admins only. This menu displays the latest **250** event logs created in the server. Logs colored green are *status* logs, *yellow* are warning logs, and *red* are error logs;

![Pasted image 20221022112545](https://user-images.githubusercontent.com/54174043/199902794-b80fbc5d-cc12-426a-9b06-5133589dbc52.png)

## Troubleshooting

Should the UI start acting weird, or requested actions not being performed without any visible error, check your console logs in the *developer console* of your browser. If you don't see anything of interest, a simple page reload may fix some issues.

Should the problem persist, try going to your *local storage* tab in the developer console of your browser, and delete an entry with the name `striker`. This is what the application uses to track your session. Reload the page and you should be back at the login page.
