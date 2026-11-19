#!/usr/bin/env python3







import base64
import json
import os
import sys

from operator import itemgetter


def read_file(path: str):
    with open(path, "rb") as f:
        return f.read()


def read_one_line(path: str):
    with open(path, "rb") as f:
        return f.readline().strip().decode("utf-8")


def main():
    diff_text = read_file("diff.txt").decode("utf-8").splitlines()

    file_hunks = parse_diff(diff_text)
    print("Files with changes: %s" % ", ".join(file_hunks.keys()))
    file_suggestions = {}
    for file, hunks in file_hunks.items():
        suggestions = []
        for hunk in hunks:
            suggestions += hunk_to_suggestions(hunk)
        file_suggestions[file] = suggestions

    for file, suggestions in file_suggestions.items():
        print(f"::group::{file}")
        print(json.dumps(suggestions, indent=2))
        print("::endgroup::")

    if len(sys.argv) > 1 and sys.argv[1] == "--dry-run":
        return

    from github import Auth, Github

    token = os.environ.get("GH_TOKEN", "")
    repo_name = os.environ.get("GITHUB_REPOSITORY", "")

    pr_number = read_one_line("pr_number.txt")
    commit_sha = read_one_line("commit_sha.txt")
    comment = read_file("comment.txt")

    if not all([token, repo_name, pr_number, commit_sha, comment]):
        print("Missing required inputs -- skipping comment posting.",
              file=sys.stderr)
        if not token:
            print("missing token", file=sys.stderr)
        if not repo_name:
            print("missing repo_name", file=sys.stderr)
        if not pr_number:
            print("missing pr_number", file=sys.stderr)
        if not commit_sha:
            print("missing commit_sha", file=sys.stderr)
        if not comment:
            print("missing comment", file=sys.stderr)
        return

    gh = Github(auth=Auth.Token(token))
    repo = gh.get_repo(repo_name)
    pr = repo.get_pull(int(pr_number))
    commit = repo.get_commit(commit_sha)

    comment_marker_tag = base64.b64encode(comment).decode("utf-8")
    comment_marker = "<!-- {}-suggest -->".format(comment_marker_tag)

    post_suggestions(
        file_suggestions,
        comment_marker,
        comment.decode("utf-8"),
        pr,
        commit,
    )
    return


def parse_diff(diff: list[str]):
    files_to_hunks = {}
    i = 0
    end = len(diff)
    while i < end:
        """
        Expecting a header of the form:

        diff --git a/<file> b/<file>
        index hash..hash mode
        --- a/file
        +++ b/file
        @@ -<old file offset>{,<len>} +<new file offset>,<len>
        <at least one line of something>

        We only really care about the old file offset
        """
        assert i + 5 < end
        assert diff[i + 0].startswith("diff --git")
        assert diff[i + 2].startswith("--- a/")
        assert diff[i + 3].startswith("+++ b/")

        file = diff[i + 2][6:]
        assert file == diff[i + 3][6:]

        hunks, i = parse_hunks(diff, i + 4)
        files_to_hunks[file] = hunks

    return files_to_hunks


def parse_hunks(diff: list[str], i: int) -> tuple[list[list[dict]], int]:
    hunks = []
    end = len(diff)
    while i < end and (header := diff[i]).startswith("@@ -"):



        offset = int(header[4:].split(' ', 1)[0].split(',')[0])
        assert offset > 0

        hunk, i = parse_hunk(diff, i + 1, offset)
        hunks.append(hunk)

    return hunks, i





def parse_hunk(diff: list[str], i: int, offset: int):
    chunks = []
    end = len(diff)
    while i < end and len(diff[i]) > 0 and (c := diff[i][0]) in " -+":
        lines = []
        start = i
        while i < end and diff[i].startswith(c):
            if c != '-':
                lines.append(diff[i][1:])
            i += 1

        length = i - start
        chunks.append({
            "type": c,
            "lines": lines,
            "length": i - start
        })
        if c != '+':
            chunks[-1]["offset"] = offset
            offset += length

    return chunks, i


def hunk_to_suggestions(hunks: list[dict[str, any]]):






    trimmed_hunks = []
    trimmed_hunk = []

    for i, hunk in enumerate(hunks):
        hunk_type, lines = itemgetter("type", "lines")(hunk)
        if hunk_type == ' ' and len(lines) > 2:
            offset = hunk["offset"]
            first_context = {
                "type": ' ',
                "offset": offset,
                "lines": lines[:1]
            }
            last_context = {
                "type": ' ',
                "offset": offset + len(lines) - 1,
                "lines": lines[-1:]
            }
            trimmed_hunk.append(first_context)
            trimmed_hunks.append(trimmed_hunk)
            trimmed_hunk = [last_context]
        else:
            trimmed_hunk.append(hunk)

    if trimmed_hunk:
        trimmed_hunks.append(trimmed_hunk)














    suggestions = []

    for hunk in trimmed_hunks:
        original_hunk = hunk
        assert len(hunk) > 0
        leading_context = None
        trailing_context = None
        if hunk[0]["type"] == ' ':
            leading_context = hunk[0]
            hunk = hunk[1:]
        if len(hunk) == 0:

            continue
        if hunk[-1]["type"] == ' ':
            trailing_context = hunk[-1]
            hunk = hunk[:-1]
        assert len(hunk) > 0

        start = None
        to = None
        lines = []
        for chunk in hunk:
            chunk_type = chunk["type"]
            if chunk_type == '+':
                lines += chunk["lines"]
                continue
            if not start:
                offset = chunk["offset"]
                start = offset
                to = offset
            to += chunk["length"]
            if chunk_type == ' ':
                lines += chunk["lines"]

        if lines and not start:

            if not leading_context and not trailing_context:

                continue
            if leading_context:
                lines = leading_context["lines"] + lines
                start = leading_context["offset"]
                to = start + leading_context["length"]
            else:
                lines = lines + trailing_context["lines"]
                start = trailing_context["offset"]
                to = start + trailing_context["length"]

        if not start or not to:

            continue


        to -= 1

        suggestions.append({
            "start": start,
            "to": to,
            "replacement": lines,
            "chunks": original_hunk,
        })

    return suggestions


def post_suggestions(
        file_suggestions: dict[str, list[dict]],
        comment_marker: str,
        comment_text: str,
        pr,
        commit,
):
    from github import GithubException
    """Post review comments with suggestion blocks for each hunk."""
    print("::group::Posting suggestions")



    for comment in pr.get_review_comments():
        if comment_marker in comment.body:
            print("Deleting stale comment %d on %s"
                  % (comment.id, comment.path))
            comment.delete()

    for path, suggestions in file_suggestions.items():
        for suggestion in suggestions:
            start = suggestion["start"]
            to = suggestion["to"]
            new_content = "\n".join(suggestion["replacement"])

            body = "%s\n%s\n```suggestion\n%s\n```" % (
                comment_marker, comment_text, new_content)

            try:
                print("Posting suggestion on %s lines %d-%d"
                      % (path, start, to))
                if start != to:
                    pr.create_review_comment(
                        body=body, commit=commit, path=path,
                        line=to, start_line=start,
                        side="RIGHT")
                else:
                    pr.create_review_comment(
                        body=body, commit=commit, path=path,
                        line=start, side="RIGHT")
            except GithubException as e:
                print("Failed to post on %s:%d - %s"
                      % (path, start, e), file=sys.stderr)

    print("::endgroup::")


if __name__ == "__main__":
    main()
