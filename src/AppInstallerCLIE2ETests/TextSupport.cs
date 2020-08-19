// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;
    using System.Security.Cryptography.X509Certificates;

    public class TextSupport
    {
        private const string TextSupportTestSourceUrl = @"https://localhost:5001/TestKit/";
        private const string TextSupportSourceName = @"TextSupportTestSource";

        [SetUp]
        public void Setup()
        {
            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("source add", $"{TextSupportSourceName} {TextSupportTestSourceUrl}").ExitCode);
        }

        [TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source remove", TextSupportSourceName);
            TestCommon.WaitForDeploymentFinish();
        }

        [Test]
        public void VerifyGB18030Support()
        {
            var result = TestCommon.RunAICLICommand("search", $"丂令龥€￥ 㐀㲷䶵 𠀀𠀁𠀂");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("丂令龥€￥ 㐀㲷䶵 𠀀𠀁𠀂"));
        }

        [Test]
        public void VerifyDirectionalitySupport()
        {
            var result = TestCommon.RunAICLICommand("search", " أنا اختبار إدخال النص في لغات مختلفة 01 لأحد منتجات Microsoft");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("أنا اختبار إدخال النص في لغات مختلفة 01 لأحد منتجات Microsoft"));
        }

        [Test]
        public void VerifyUnicodeSupport()
        {
            var result = TestCommon.RunAICLICommand("show", $"丂令龥€￥ 㐀㲷䶵 𠀀𠀁𠀂 -s {TextSupportSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("İkşzlerAçık芲偁ＡＢＣ巢für नमस्ते กุ้งจิ้яЧчŠš𠀀𠀁𠀂"));
        }
    }
}
